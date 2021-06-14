# -*- coding: utf-8 -*-


class Token(object):

    ident = 1
    integer =2
    char = 3
    string = 4
    graphic = 5
    NL = 6
    EOF = 7

    def __init__(self, line, row, col):
        self.line, self.row, self.col = line, row, col
        self.__buf = []
        self.__str = None
        self.t_type = None

    def __lshift__(self, c):
        if c is None:
            raise Exception('Unexpceted EOF encountered')
        self.__buf.append(c)
        self.__str = None
        return self

    @property
    def s(self):
        if self.__str is None:
            self.__str = ''.join(self.__buf)
        return self.__str

    @property
    def is_ident(self):
        return self.t_type == self.ident

    @property
    def is_integer(self):
        return self.t_type == self.integer

    @property
    def is_NL(self):
        return self.t_type == self.NL

    @property
    def is_EOF(self):
        return self.t_type == self.EOF

    def __eq__(self, s):
        return (self.s == s)

    def __ne__(self, s):
        return (self.s != s)

    def __repr__(self):
        return "token<'%s', row:%d, col:%d>" % (self.s, self.row, self.col)
 
    __str__ = __repr__

    def __nonzero__(self):
        return bool(self.__buf)


class Reader(object):

    def __init__(self, path):
        self.__fo = open(path)
        self.__line = ''
        self.__row = 0
        self.__col = 0

        def scan():
            for s in self.__fo.readlines():
                self.__line = s
                self.__row += 1
                self.__col = 0
                for i, c in enumerate(s):
                    self.__col = i
                    yield c
            while True:
                yield None
        self.__iter = iter(scan())

    def get(self):
        return next(self.__iter)

    def token(self, ch=None, t_type=None):
        t = Token(self.__line, self.__row, self.__col)
        t.t_type = t_type
        if ch is not None:
            t << ch
        return t

    def __str__(self):
        return 'reader<row:%d, col:%d>' % (self.__row, self.__col)


def make_graphics_dic(g_list):
    def build_dic(dic, s):
        if s:
            d = dic.setdefault(s[0], dict())
            build_dic(d, s[1:])
    top_dic = dict()
    for g in g_list:
        build_dic(top_dic, g)
    return top_dic


class WeakTokenizer(object):

    def __init__(self, path, special_comment=False):
        self.__special_comment = special_comment
        self.__reader = Reader(path)
        self.__spaces = set(" \t\f")
        self.__id_head = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")
        self.__id_body = set(self.__id_head)
        self.__id_body.update(set("0123456789"))
        self.__digits = set("0123456789")
        self.__hexdigits = set("0123456789abcdefABCDEF")
        self.__gdic = make_graphics_dic([
            '/*',	# block comment
            '//',	# line comment
            '//[',	# line comment (but special treatment)
            '\\',
            '#',
            '{',
            '}',
            '(',
            ')',
            '[',
            ']',
            '++',
            '--',
            '.',
            '.*',
            '->',
            '->*',
            '&',
            '*',
            '~',
            '!',
            '/',
            '%',
            '+',
            '-',
            '<<',
            '>>',
            '<',
            '<=',
            '>',
            '>=',
            '==',
            '!=',
            '^',
            '|',
            '&&',
            '||',
            '?',
            ':',
            '::',
            ';',
            '=',
            '*=',
            '/=',
            '%=',
            '+=',
            '-=',
            '<<=',
            '>>=',
            '&=',
            '^=',
            '|=',
            ','])
        self.next_c

    @property
    def c(self):
        return self.__ch

    @property
    def next_c(self):
        self.__ch = self.__reader.get()
        return self.__ch

    @property
    def c_next(self):
        ch = self.__ch
        self.__ch = self.__reader.get()
        return ch

    def __skip_space(self):
        while self.c in self.__spaces:
            self.next_c
        return self.c

    def __skip_block_comment(self):
        while self.c is not None:
            if self.c_next == '*':
                if self.c_next == '/':
                    return

    def __skip_line_comment(self):
        while self.c is not None:
            if self.c == '\n':
                return
            self.next_c

    def __ident(self):
        token = self.__reader.token(self.c, Token.ident)
        c = self.next_c
        while c in self.__id_body:
            token << c
            c = self.next_c
        if c == "'" and token.s in ('u', 'U', 'L'):
            token.t_type = Token.char
            return self.__quooted("'", token)
        elif c == '"'and token.s in ('u8'  'u'  'U'  'L'):
            token.t_type = Token.string
            return self.__quooted('"', token)
        elif c == '"'and token.s in ('u8R'  'uR'  'UR'  'LR'):
            token.t_type = Token.string
            return self.__raw_quoted(token)
        return token

    def __integer(self):
        token = self.__reader.token(self.c, Token.integer)
        dic = self.__digits
        if self.c_next == '0':
            if self.c in 'xX':
                token << self.c_next
                dic = self.__hexdigits
        while self.c in dic:
            token << self.c_next
        return token

    def __quoted(self, q_ch, token):
        token << self.c_next
        while True:
            c = self.c_next
            token << c
            if c == '\\':
                token << self.c_next
            elif c == q_ch:
                return token

    def __raw_quoted(self, prefix):
        token = self.__reader.token(prefix, Token.string)
        token << self.c_next
        marker = []
        while self.c != '(':
            token << self.c
            marker.append(self.c_next)
        marker = ''.join(marker) + ')"'
        while not token.s.endswith(marker):
            token << self.c_next
        return token

    def __char(self):
        token = self.__reader.token(t_type=Token.char)
        return self.__quoted("'", token)

    def __string(self):
        token = self.__reader.token(t_type=Token.string)
        return self.__quoted('"', token)

    def __graphics(self):
        token = self.__reader.token(t_type=Token.graphic)
        def build(dic):
            while self.c in dic:
                token << self.c
                dic = dic[self.c_next]
        build(self.__gdic)
        if not token:
            raise Exception('Invalid character: %s (%s)' % (self.c, token))
        return token

    def __newline(self):
        token = self.__reader.token('\n', t_type=Token.NL)
        self.next_c
        return token

    def __eof(self):
        return self.__reader.token(t_type=Token.EOF)

    def get(self):
        while True:
            c = self.__skip_space()
            if c in self.__id_head:
                return self.__ident()
            if c in self.__digits:
                return self.__integer()
            if c == "'":
                return self.__char()
            if c == '"':
                return self.__string()
            if c == '\n':
                return self.__newline()
            if c is None:
                return self.__eof()
            t = self.__graphics()
            if t == '/*':
                self.__skip_block_comment()
            elif t == '//':
                self.__skip_line_comment()
            elif t == '//[' and not self.__special_comment:
                self.__skip_line_comment()
            else:
                return t
