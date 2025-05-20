#!/usr/ebsys/opt/bin/python3

from cxx_token import WeakTokenizer, Token

import os
import re
import sys


class ParseError(Exception):
    def __new__(cls, fmt, *args):
        return super().__new__(cls, fmt % args)

    def __init__(self, *args):
        pass


def is_basic_type(type_name):
    basic_types = ('int', 'real', 'str', 'bool', 'bin', 'usec', 'tagged_int', 'tagged_str')
    return type_name in basic_types


class Type(object):
    def __init__(self):
        self.__stack = []

    def ident(self, type_name):
        if is_basic_type(type_name):
            type_name = 'c7::json_' + type_name
        self.__stack.append(type_name)

    def to_array(self):
        tn = self.__stack.pop()
        self.__stack.append('c7::json_array<%s>' % tn)

    def to_dict(self):
        vn = self.__stack.pop()
        kn = self.__stack.pop()
        self.__stack.append('c7::json_dict<%s, %s>' % (kn, vn))

    def to_tuple(self):
        ts = []
        while True:
            t = self.__stack.pop()
            if not t:
                break
            ts.append(t)
        self.__stack.append('c7::json_tuple<%s>' % ', '.join(reversed(ts)))

    def to_pair(self):
        sn = self.__stack.pop()
        fn = self.__stack.pop()
        self.__stack.append('c7::json_pair<%s, %s>' % (fn, sn))

    def to_templated(self):
        ts = []
        while True:
            t = self.__stack.pop()
            if not t:
                break
            ts.append(t)
        tn = self.__stack.pop()
        self.__stack.append('%s<%s>' % (tn, ', '.join(reversed(ts))))

    @property
    def name(self):
        return self.__stack[0]

    def __repr__(self):
        return repr(self.__stack)


class JsonAlias(object):
    def __init__(self, name):
        self.name = name
        self.real_type = None

    def put(self, out):
        out.write('using %s = %s;\n\n' % (self.name, self.real_type.name))


class JsonObject(object):
    def __init__(self, name, obj_type):
        self.name     = name
        self.obj_type = obj_type
        self.members  = []	# (Type, member_name), ...

    def put(self, out):
        def p_(fmt, *args):
            if args:
                out.write(fmt % args)
            else:
                out.write(fmt)

        p_('struct %s: public c7::json_%s {\n', self.name, self.obj_type)
        for t, mn in self.members:
            p_('    %s %s;\n', t.name, mn)

        # invoke constructor of base class
        p_('\n    using c7::json_%s::json_%s;\n\n', self.obj_type, self.obj_type)

        # constructor
        mbrs = list(enumerate(self.members))
        if True:
            def tmpl_args(mbrs):
                for i, (t, mn) in mbrs:
                    if i == 0:
                        yield 'typename T%d' % i
                    else:
                        yield 'typename T%d=%s' % (i, t.name)

            def args(mbrs):
                for i, (t, mn) in mbrs:
                    if i == 0:
                        yield 'T%d&& a_%s' % (i, mn)
                    else:
                        yield 'T%d&& a_%s=T%d()' % (i, mn, i)

            next_prefix = ',\n%*s' % (14, '')
            p_('    template <%s>\n',
               next_prefix.join(tmpl_args(mbrs)))

            next_prefix = ',\n%*s' % (14 + len(self.name), '')
            p_('    explicit %s(%s):\n\t%s {}\n',
               self.name,
               next_prefix.join(args(mbrs)),
               ',\n\t'.join('%s(std::forward<T%d>(a_%s))' % (mn, i, mn) for i, (_, mn) in mbrs))
        else:
            def args(mbrs):
                for i, (t, mn) in mbrs:
                    if i == 0:
                        yield 'const %s& a_%s' % (t.name, mn)
                    else:
                        yield 'const %s& a_%s=%s()' % (t.name, mn, t.name)
            next_prefix = ',\n%*s' % (14 + len(self.name), '')
            p_('    explicit %s(%s):\n\t%s {}\n',
               self.name,
               next_prefix.join(args(mbrs)),
               ',\n\t'.join('%s(a_%s)' % (mn, mn) for _, (_, mn) in mbrs))

        # operator==, operator!=
        p_('\n    bool operator==(const %s& o) const {\n', self.name)
        p_('        return (')
        p_(' &&\n                '.join('%s == o.%s' % (mn, mn) for i, (t, mn) in mbrs))
        p_(');\n    }\n')
        p_('\n    bool operator!=(const %s& o) const { return !(*this == o); }\n', self.name)

        # c7json_init
        p_('\n    c7json_init(\n')
        for _, (_, mn) in mbrs:
            p_('        c7json_member(%s),\n', mn)
        p_('        )\n')
        p_('};\n\n')


def expect_list(*args):
    token_category = ('', 'ident', 'integer', 'char', 'string', 'graphic', 'NL', 'EOF')
    for e in args:
        if isinstance(e, str):
            yield e
        else:
            yield token_category[e.t_type]


class Tokenizer(object):
    def __init__(self, path):
        self.__tokenizer = WeakTokenizer(path, special_comment=True)

    def get(self):
        while True:
            tk = self.__tokenizer.get()
            if not tk.is_NL:
                return tk

    def expect(self, *args):
        tk = self.get()
        for e in args:
            if isinstance(e, str):
                if tk.s == e:
                    return tk
            else:
                if tk.t_type == e:	# Token.ident, ...
                    return tk
        raise ParseError('Unexpected token:%s, expect:%s', tk, list(expect_list(*args)))


class Parser(object):

    TYPE_HEAD = (Token.ident, '{', '[', '(', '<')

    def __init__(self, path):
        self.__tokenizer = Tokenizer(path)
        self.json_defs_list = []
        self.json_defs = []

    def __get(self):
        self.tk = self.__tokenizer.get()
        return self.tk

    def __expect(self, *args):
        self.tk = self.__tokenizer.expect(*args)
        return self.tk

    def __skip_to_defend(self, tk):
        while tk:
            if tk == '*/':
                return self.__get()
            tk = self.__get()
        return tk

    def __skip_to_defbeg(self, tk):
        while tk:
            if tk == '/*[':
                if (self.__get() == 'c7json' and
                    self.__get() == ':' and
                    self.__get() == 'define' and
                    self.__get() == ']'):
                    return self.__expect(Token.ident, '*/')
                else:
                    tk = self.__skip_to_defend(tk)
            else:
                tk = self.__get()
        return tk

    def __parse_json_type(self, json_type, tk):
        # tkn is one of ident, '{', '[', '(', '<'
        if tk.is_ident:
            json_type.ident(tk.s)
            if self.__get() == '<':
                json_type.ident('')
                while True:
                    self.__expect(*self.TYPE_HEAD)
                    tk = self.__parse_json_type(json_type, self.tk)
                    if tk != ',':
                        if tk != '>':
                            raise ParseError('> is expected:%s', tk)
                        break
                json_type.to_templated()
                self.__get()
            return self.tk
        elif tk == '{':
            self.__expect(*self.TYPE_HEAD)
            tk = self.__parse_json_type(json_type, self.tk)
            if tk != '->':
                raise ParseError('colon(:) is expected:%s', tk)
            self.__expect(*self.TYPE_HEAD)
            tk = self.__parse_json_type(json_type, self.tk)
            if tk != '}':
                raise ParseError(') is expected:%s', tk)
            json_type.to_dict()
        elif tk == '[':
            self.__expect(*self.TYPE_HEAD)
            tk = self.__parse_json_type(json_type, self.tk)
            if tk != ']':
                raise ParseError('] is expected:%s', tk)
            json_type.to_array()
        elif tk == '(':
            json_type.ident('')
            while True:
                self.__expect(*self.TYPE_HEAD)
                tk = self.__parse_json_type(json_type, self.tk)
                if tk != ',':
                    if tk != ')':
                        raise ParseError(') is expected:%s', tk)
                    break
            json_type.to_tuple()
        elif tk == '<':
            self.__expect(*self.TYPE_HEAD)
            tk = self.__parse_json_type(json_type, self.tk)
            if tk != ',':
                raise ParseError('comma(,) is expected:%s', tk)
            self.__expect(*self.TYPE_HEAD)
            tk = self.__parse_json_type(json_type, self.tk)
            if tk != '>':
                raise ParseError('> is expected:%s', tk)
            json_type.to_pair()
        return self.__get()

    def __parse_json_alias(self, name):
        alias = JsonAlias(name)
        self.json_defs.append(alias)
        self.__expect(*self.TYPE_HEAD)
        json_type = Type()
        self.__parse_json_type(json_type, self.tk)
        alias.real_type = json_type
        if self.tk != ';':
            raise ParseError('semi colon(;) is expected:%s', self.tk)
        return self.__get()

    def __parse_json_obj(self, tk):
        # tkn is ident
        name = tk.s

        obj_type = 'object'
        if self.__expect('!', '{', '=') == '=':
            return self.__parse_json_alias(name)
        if self.tk == '!':
            obj_type = 'struct'
            self.__expect('{')

        # '{' 
        jso = JsonObject(name, obj_type)
        self.json_defs.append(jso)

        tk = self.__expect(*self.TYPE_HEAD, '}')
        while tk != '}':
            json_type = Type()
            tk = self.__parse_json_type(json_type, tk)
            if not tk.is_ident:
                raise ParseError('Identifier is expected:%s', tk)
            jso.members.append((json_type, tk.s))
            self.__expect(';')
            tk = self.__expect(*self.TYPE_HEAD, '}')
        # tk is '}'
        return self.__expect(Token.ident, '*/')

    def __parse_json_define(self, tk):
        # tkn is ident or '*/'
        while tk.is_ident:
            tk = self.__parse_json_obj(tk)
        # tkn is '*/'
        return self.__get()

    def parse(self):
        self.__get()
        while self.tk:
            if self.__skip_to_defbeg(self.tk):
                self.__parse_json_define(self.tk)
                self.json_defs_list.append(self.json_defs)
                self.json_defs = []


class Replacer(object):

    def __init__(self, path, json_defs_list):
        self.__path = path
        self.__tmppath = path + ".tmp"
        self.__rf = open(path)
        self.__wf = open(self.__tmppath, "w")
        self.__beg = '//[c7json:begin]'
        self.__end = '//[c7json:end]'
        self.__json_defs_list = json_defs_list

    def run(self):
        skip = False
        replaced = False

        for s in self.__rf:
            if s.startswith(self.__beg):
                self.__wf.write(s)
                skip = True
                if self.__json_defs_list:
                    json_defs = self.__json_defs_list.pop(0)
                    if json_defs:
                        replaced = True
                        self.__wf.write('\n')
                        for js in json_defs:
                            js.put(self.__wf)
                else:
                    pass
            elif s.startswith(self.__end):
                self.__wf.write(s)
                skip = False
            elif not skip:
                self.__wf.write(s)

        self.__rf.close()
        self.__wf.close()
        if replaced and (open(self.__path).read() !=
                         open(self.__tmppath).read()):
            os.rename(self.__tmppath, self.__path)
        else:
            os.remove(self.__tmppath)


if __name__ == '__main__':
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print('''Usage: %s input [output]''' % sys.argv[0])
        exit(1)

    path = sys.argv[1]
    parser = Parser(path)
    parser.parse()
    if parser.json_defs_list:
        replacer = Replacer(path, parser.json_defs_list)
        replacer.run()
