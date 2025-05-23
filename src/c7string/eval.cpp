/*
 * c7string/eval.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <cstring>
#include <c7string/c_str.hpp>
#include <c7string/eval.hpp>


namespace c7::str {


/*----------------------------------------------------------------------------
       eval C backslash sequece: convert C backslash sequence to a byte
----------------------------------------------------------------------------*/

static inline bool isoctal(char ch)
{
    return (std::strchr("01234567", ch) != nullptr);
}

std::string& eval_C(std::string& out, const std::string& s)
{
    const char *in = s.c_str();

    while (*in != 0) {
	if (*in != '\\') {
	    out += *in++;
	} else {
	    const char *p, * const cvlist = "\\\\t\ta\007n\nf\fr\r";
	    p = std::strchr(cvlist, *++in);
	    if (*in == 0) {
		out += '\\';
	    } else if (p != nullptr && ((p - cvlist) & 0x1) == 0) {
		out += p[1];
		in++;
	    } else if (*in == 'x' && std::isxdigit(in[1])) {
		out += static_cast<char>(std::strtol(in+1, (char **)&in, 16));
	    } else if (isoctal(*in)) {
		int ch = *in++ - '0';
		if (isoctal(*in)) {
		    ch = (ch << 3) + *in++ - '0';
		}
		if (isoctal(*in)) {
		    ch = (ch << 3) + *in++ - '0';
		}
		out += static_cast<char>(ch);
	    } else {
		out += *in++;
	    }
	}
    }
    return out;
}

std::ostream& eval_C(std::ostream& out, const std::string& s)
{
    std::string tmp;
    (void)eval_C(tmp, s);
    out.write(tmp.c_str(), tmp.size());
    return out;
}

std::string eval_C(const std::string& s)
{
    std::string out;
    (void)eval_C(out, s);
    return out;
}


/*----------------------------------------------------------------------------
               escape_C: convert a byte to C backslash sequence
----------------------------------------------------------------------------*/

std::string& escape_C(std::string& out, const std::string& s, char quote)
{
    const char *in = s.c_str();
    static const char cvlist[] = "\tt\aa\nn\ff\rr";

    for (; *in != 0; in++) {
	if (*in == quote || *in == '\\') {
	    out += '\\';
	    out += *in;
	} else if (std::isgraph(*in) || *in == ' ') {
	    out += *in;
	} else if (auto p = std::strchr(cvlist, *in); p != nullptr) {
	    out += '\\';
	    out += p[1];
	} else {
	    c7::format(out, "\\x%{02x}", static_cast<unsigned char>(*in));
	}
    }
    return out;
}

std::ostream& escape_C(std::ostream& out, const std::string& s, char quote)
{
    std::string tmp;
    (void)escape_C(tmp, s, quote);
    out.write(tmp.c_str(), tmp.size());
    return out;
}

std::string escape_C(const std::string& s, char quote)
{
    std::string out;
    (void)escape_C(out, s, quote);
    return out;
}


/*----------------------------------------------------------------------------
                          eval (customization base)
----------------------------------------------------------------------------*/

struct evalprm {
    char mark;
    char escape;
    char begin;
    char end;
    const char *brks;
    c7::str::evalvar evalvar;
};

static std::string& append(std::string& out, const char *s, const char *e)
{
    return out.append(s, e-s);
}

static result<const char*> evalvarref(std::string& out, const char *in, const evalprm& prm)
{
    if (*in != prm.begin) {
	// evalvar() must return a pointer of next char of VARNAME
	return prm.evalvar(out, in, false);
    }

    std::string var;
    const char *refbeg = in - 1;
    result<const char*> res;

    in++;
    const char *p;
    while ((p = std::strpbrk(in, prm.brks)) != nullptr) {
	var.append(in, p - in);
	if (*p == prm.escape) {
	    if (p[1] == 0) {
		return c7result_seterr(res, "Invalid escape sequence: '%{}'", refbeg);
	    }
	    var += p[1];
	    in = p + 2;
	} else if (*p == prm.mark) {
	    res = evalvarref(var, p + 1, prm);
	    if (!res) {
		return res;
	    }
	    in = res.value();
	} else if (*p == prm.end) {
	    res = prm.evalvar(out, var.c_str(), true);
	    if (!res) {
		return res;
	    }
	    res = p + 1;
	    return res;			// success
	} else {
	    var += *p;
	    in = p + 1;
	}
    }
    return c7result_seterr(res, "No closing character '%{}': '%{}'", prm.end, refbeg);
}

c7::result<> eval(std::string& out, const std::string& in_, char mark, char escape,
		  c7::str::evalvar evalvar)
{
    const char brks[] = { mark, escape, '}', 0 };
    evalprm prm = {
	.mark = mark,
	.escape = escape,
	.begin = '{',
	.end = '}',
	.brks = brks,
	.evalvar = evalvar,
    };

    c7::result<const char*> res;

    const char *in = in_.c_str();
    const char *p;
    while ((p = std::strpbrk(in, prm.brks)) != nullptr) {
	append(out, in, p);
	if (*p == escape && p[1] == mark) {
	    out += mark;
	    in = p + 2;
	} else if (*p == mark) {
	    res = evalvarref(out, p + 1, prm);
	    if (!res) {
		return res.as_error();
	    }
	    in = res.value();
	} else {
	    out += *p;
	    in = p + 1;
	}
    }

    append(out, in, std::strchr(in, 0));
    return c7result_ok();
}

c7::result<> eval(std::ostream& out, const std::string& in, char mark, char escape,
		  c7::str::evalvar evalvar)
{
    std::string tmp;
    auto r = eval(tmp, in, mark, escape, evalvar);
    if (r) {
	out.write(tmp.c_str(), tmp.size());
    }
    return r;
}


c7::result<std::string> eval(const std::string& in, char mark, char escape,
			     c7::str::evalvar evalvar)
{
    std::string out;
    auto r = eval(out, in, mark, escape, evalvar);
    if (r) {
	return c7result_ok(std::move(out));
    }
    return r.as_error();
}


/*----------------------------------------------------------------------------
               eval shell-syntax environment variable reference
----------------------------------------------------------------------------*/

static result<const char*> evalenv(std::string& out, const char *vn, bool enclosed)
{
    const char *ve;

    if (enclosed) {
	ve = std::strchr(vn, 0);
	const char *m = strchr_x(vn, ':', ve);
	std::string env(vn, m - vn);
	const char *val = getenv_x(env.c_str(), "");
	if (*m == 0) {		// m == ve
	    out += val;
	} else if (m[1] == '+') {
	    if (*val != 0) {
		out += m + 2;
	    }
	} else if (m[1] == '-') {
	    out += (*val == 0) ? m+2 : val;
	} else {
	    out += "${";
	    out += vn;
	    out += '}';
	}
    } else {
	ve = vn + 1;
	if (std::isalpha(*vn) || *vn == '_') {
	    for (; std::isalnum(*ve) || *ve == '_'; ve++);
	}
	std::string env(vn, ve - vn);
	const char *val = getenv_x(env.c_str(), "");
	out += val;
    }

    return result<const char*>(ve);
}

c7::result<> eval_env(std::string& out, const std::string& in)
{
    return eval(out, in, '$', '\\', evalenv);
}

c7::result<> eval_env(std::ostream& out, const std::string& in)
{
    return eval(out, in, '$', '\\', evalenv);
}

c7::result<std::string> eval_env(const std::string& in)
{
    return eval(in, '$', '\\', evalenv);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::str


