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


class Json(object):
    class Type(object):
        def __init__(self, type_name):
            basic_types = ('int', 'real', 'str', 'bool', 'bin', 'usec')
            if type_name in basic_types:
                type_name = 'c7::json_' + type_name
            self.name = type_name

        def to_array(self):
            self.name = 'c7::json_array<' + self.name + '>'

    def __init__(self, name):
        self.name = name
        self.members = []	# (Type, member_name), ...

    def put(self, out):
        def p_(fmt, *args):
            out.write(fmt % args)

        p_('struct %s: public c7::json_object {\n', self.name)
        for t, mn in self.members:
            p_('    %s %s;\n', t.name, mn)

        # invoke constructor of base class
        p_('\n    using c7::json_object::json_object;\n\n')

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

    def __init__(self, path):
        self.__tokenizer = Tokenizer(path)
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

    def __parse_json_obj(self, tk):
        # tkn is ident
        jso = Json(tk.s)
        self.json_defs.append(jso)
        self.__expect('{')
        tk = self.__expect(Token.ident, '}')
        while tk.is_ident:
            json_type = Json.Type(tk.s)
            if self.__expect(Token.ident, '[') == '[':
                json_type.to_array()
                self.__expect(']')
                if self.__expect(Token.ident, '[') == '[':
                    json_type.to_array()
                    self.__expect(']')
                    self.__expect(Token.ident)
            tk = self.tk
            jso.members.append((json_type, tk.s))
            self.__expect(';')
            tk = self.__expect(Token.ident, '}')
        # tk is '}'
        return self.__expect(Token.ident, '*/')

    def __parse_json_define(self, tk):
        # tkn is ident or '*/'
        while tk.is_ident:
            tk = self.__parse_json_obj(tk)
        # tkn is '*/'
        return self.__get()

    def parse(self):
        if self.__skip_to_defbeg(self.__get()):
            self.__parse_json_define(self.tk)


class Replacer(object):

    def __init__(self, path, json_defs):
        self.__path = path
        self.__tmppath = path + ".tmp"
        self.__rf = open(path)
        self.__wf = open(self.__tmppath, "w")
        self.__beg = '//[c7json:begin]'
        self.__end = '//[c7json:end]'
        self.__json_defs = json_defs

    def run(self):
        replaced = False
        skip = False

        for s in self.__rf:
            if s.startswith(self.__beg) and not replaced:
                self.__wf.write(s)
                replaced = True
                skip = True
                if self.__json_defs:
                    self.__wf.write('\n')
                for js in self.__json_defs:
                    js.put(self.__wf)
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
    if parser.json_defs:
        replacer = Replacer(path, parser.json_defs)
        replacer.run()
