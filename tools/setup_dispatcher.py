# -*- coding: utf-8 -*-


from cxx_token import WeakTokenizer
import os
import re
import sys


class CxxClass(object):

    def __init__(self, cls_name):
        self.name = cls_name
        self.__callbacks = []	# (callback_name, ev_beg, eb_end) or prep_line

    def add_prep(self, line):
        self.__callbacks.append(line)

    def add_callback(self, callback_name, ev_beg=None, ev_end=None):
        self.__callbacks.append((callback_name, ev_beg, ev_end))

    def put_callbacks(self, fobj):
        for data in self.__callbacks:
            if isinstance(data, str):
                fobj.write(data)
                continue
            name, ev_beg, ev_end = data
            if ev_beg is None:
                ev_beg = ev_end = name
                fobj.write('    this->dispatcher_set(%s, &%s::callback_%s);\n' %
                           (name, self.name, name))
            elif ev_end is None:
                fobj.write('    this->dispatcher_set(%s, &%s::callback_%s);\n' %
                           (ev_beg, self.name, name))
            else:
                fobj.write('    this->dispatcher_set(%s, %s, &%s::callback_%s);\n' %
                           (ev_beg, ev_end, self.name, name))


class Parser(object):

    def __init__(self, hpp_path):
        self.__tokenizer = WeakTokenizer(hpp_path, special_comment=True)

    def __get(self):
        return self.__tokenizer.get()

    def __skip_to_eol(self):
        tk = self.__get()
        while tk and tk != '\n':
            tk = self.__get()
        return tk

    def __parse_attr(self, evlist):
        tk = self.__get()
        if tk != 'dispatcher':
            return self.__skip_to_eol()
        tk = self.__get()
        if tk != ':':
            return self.__skip_to_eol()
        tk = self.__get()
        if tk != 'callback':
            return self.__skip_to_eol()

        tk = self.__get()
        while True:
            if tk.is_ident or tk.is_integer:
                beg = tk
                end = None
                tk = self.__get()
                if tk == ':':
                    tk = self.__get()
                    if tk.is_ident or tk.is_integer:
                        end = tk
                    else:
                        raise Exception('ev_beg:ev_end syntax error: %s' % tk)
                    tk = self.__get()
                evlist.append((beg, end))
            elif tk == ',':
                tk = self.__get()
            elif tk == ']':
                return self.__skip_to_eol()
            else:
                raise Exception('ev_beg:ev_end syntax error: %s' % tk)

    def __parse_class(self, tk, cls):
        nest = 0
        while tk:
            tk = self.__get()
            if tk == '}':
                nest -= 1
                if nest == 0:
                    return self.__get()
            elif tk == '{':
                nest += 1
                evlist = []
            elif tk == ';' or tk == '(':
                evlist = []
            elif tk == '//[' and nest == 1:
                tk = self.__parse_attr(evlist)
            elif tk == '#' and nest == 1:
                tk = self.__get()
                if tk.s in ('if', 'ifdef', 'ifndef', 'elif', 'else', 'endif'):
                    cls.add_prep(tk.line)
                tk = self.__skip_to_eol()
            elif tk.is_ident and nest == 1:
                ident = tk.s
                if ident.startswith('callback_') and ident != 'callback_default':
                    name = ident[len('callback_'):]
                    if evlist:
                        for beg, end in evlist:
                            if end is None:
                                cls.add_callback(name, beg.s, None)
                            else:
                                cls.add_callback(name, beg.s, end.s)
                        evlist = []
                    else:
                        cls.add_callback(name)

    def parse(self):
        clss = []

        tk = self.__get()
        while tk:
            while tk != 'class':
                tk = self.__get()
                if not tk:
                    return clss
            tk = self.__get()
            if not tk.is_ident:
                raise Exception('class name syntax error: %s' % tk)

            cls = CxxClass(tk.s)
            clss.append(cls)
            tk = self.__parse_class(tk, cls)

        return clss


class Replacer(object):

    def __init__(self, path, clss):
        self.__path = path
        self.__tmppath = path + ".tmp"
        self.__rf = open(path)
        self.__wf = open(self.__tmppath, "w")
        self.__clss = clss

        pattern = r'[ \t]*(void)?[ \t]*([a-zA-Z_]\w+)(<.*>)?::dispatcher_setup\(\)'
        self.__reg = re.compile(pattern)
        self.__beg = '//[dispatcher:setup begin]'
        self.__end = '//[dispatcher:setup end]'

    def __find(self, cls_name):
        for cls in self.__clss:
            if cls.name == cls_name:
                return cls
        return None

    def run(self):
        replaced = False
        cls = None
        skip = False
        for line in self.__rf:
            m = self.__reg.match(line)
            if m:
                self.__wf.write(line)
                cls = self.__find(m.group(2))
            elif self.__beg in line:
                self.__wf.write(line)
                if cls:
                    replaced = True
                    cls.put_callbacks(self.__wf)
                    skip = True
            elif self.__end in line:
                self.__wf.write(line)
                cls = None
                skip = False
            elif not skip:
                self.__wf.write(line)
        self.__rf.close()
        self.__wf.close()
        if replaced and (open(self.__path).read() !=
                         open(self.__tmppath).read()):
            os.rename(self.__tmppath, self.__path)
        else:
            os.remove(self.__tmppath)


if __name__ == '__main__':
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print('''Usage: %s input [output]
        input  ... C++ file declare your service class.
        output ... C++ file define dispatcher_setup() [out of class declaration]''' % sys.argv[0])
        exit(1)

    in_cxx = out_cxx = sys.argv[1]
    if len(sys.argv) == 3:
        out_cxx = sys.argv[2]
    clss = Parser(in_cxx).parse()
    if clss:
        Replacer(out_cxx, clss).run()
