/*
 * c7json/proxy.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unordered_set>
#include <c7nseq/base64.hpp>
#include <c7nseq/push.hpp>
#include <c7json/proxy.hpp>


namespace c7::json {


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

// integer

template <> c7::result<>
proxy_basic<int64_t>::load(lexer&, token& tkn)
{
    if (tkn.code == TKN_INT) {
	val_ = tkn.i64();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "Integer is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic<int64_t>::dump(std::ostream& o, dump_context&)
{
    o << std::dec << val_;
    return c7result_ok();
}


// real number

template <> c7::result<>
proxy_basic<double>::load(lexer&, token& tkn)
{
    if (tkn.code == TKN_REAL) {
	val_ = tkn.r64();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "Real number is expected: %{}", tkn);
}

template <> c7::result<>
proxy_basic<double>::dump(std::ostream& o, dump_context&)
{
    o << std::defaultfloat << val_;
    return c7result_ok();
}


// bool

template <> c7::result<>
proxy_basic<bool>::load(lexer&, token& tkn)
{
    if (tkn.code == TKN_TRUE) {
	val_ = true;
	return c7result_ok();
    }
    if (tkn.code == TKN_FALSE) {
	val_ = false;
	return c7result_ok();
    }
    return c7result_err(EINVAL, "Boolean value true/false is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic<bool>::dump(std::ostream& o, dump_context&)
{
    o << std::boolalpha << val_;
    return c7result_ok();
}


// string

template <> c7::result<>
proxy_basic<std::string>::load(lexer&, token& tkn)
{
    if (tkn.code == TKN_STRING) {
	val_ = tkn.str();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "String \"...\" is expected: %{}", tkn);
}


static void jsonize_string(std::ostream& o, const std::string& s)
{
    o << '"';
    for (auto c: s) {
	switch (c) {
	case '\b': o << "\\b"; break;
	case '\f': o << "\\f"; break;
	case '\n': o << "\\n"; break;
	case '\r': o << "\\r"; break;
	case '\t': o << "\\t"; break;
	case '\\': o << "\\\\"; break;
	case '/' : o << "\\/" ; break;
	case '"' : o << "\\\""; break;
	default:
	    if (static_cast<uint32_t>(c) < 0x20) {
		c7::format(o, "\\u%{04x}", static_cast<uint32_t>(c));
	    } else {
		o << c;
	    }
	}
    }
    o << '"';
}

template <> c7::result<>
proxy_basic<std::string>::dump(std::ostream& o, dump_context&)
{
    jsonize_string(o, val_);
    return c7result_ok();
}


// time_us

template <> c7::result<>
proxy_basic<time_us>::load(lexer&, token& tkn)
{
    if (auto res = tkn.as_time(); res) {
	val_ = res.value();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "ISO8601 Date and time string is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic<time_us>::dump(std::ostream& o, dump_context& dc)
{
    c7::usec_t  tv = val_;
    time_t   tv_s  = tv / C7_TIME_S_us;
    uint32_t tv_us = tv % C7_TIME_S_us;
    uint32_t tv_ms = tv_us / 1000;

    std::string tmfmt = "\"%Y-%m-%dT%H:%M:%S";

    const char *fmt = ".%{0*d}%%z\"";
    if (dc.time_prec == 6) {
	tmfmt += c7::format(fmt, 6, tv_us);
    } else if (dc.time_prec == 3) {
	tmfmt += c7::format(fmt, 3, tv_ms);
    }

    struct tm tms;
    localtime_r(&tv_s, &tms);
    char buf[64];
    strftime(buf, sizeof(buf), tmfmt.c_str(), &tms);

    char *p = strchr(buf, 0) - 3;	// ...  'm'  'm'  '"'  '\0'
    memmove(p+1, p, 4);			// ...  'm'  'm'  'm'  '"'  '\0'
    *p = ':';				// ...  ':'  'm'  'm'  '"'  '\0'
    o << buf;

    return c7result_ok();
}


// binary_t (std::vector<uint8_t>)

template <> c7::result<>
proxy_basic<binary_t>::load(lexer&, token& tkn)
{
    if (tkn.code == TKN_STRING) {
	binary_t ba;
	tkn.str() | c7::nseq::base64dec() | c7::nseq::push_back(ba);
	val_ = std::move(ba);
	return c7result_ok();
    }
    return c7result_err(EINVAL, "BASE64 encoded string \"...\" is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic<binary_t>::dump(std::ostream& o, dump_context&)
{
    binary_t empty;
    std::string s;
    val_ | c7::nseq::base64enc() | c7::nseq::push_back(s);
    jsonize_string(o, s);
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                 proxy_array
----------------------------------------------------------------------------*/

c7::result<>
proxy_array_base::load(lexer& lxr, token& t)
{
    if (t.code != TKN_SQUARE_L) {
	return c7result_err(EINVAL, "'[' is expected: %{}", t);
    }
    if (lxr.get(t) != TKN_SQUARE_R) {
	for (;;) {
	    if (auto res = load_element(lxr, t); !res) {
		return res;
	    }
	    if (lxr.get(t) != TKN_COMMA) {
		break;
	    }
	    lxr.get(t);
	}
	if (t.code != TKN_SQUARE_R) {
	    return c7result_err(EINVAL, "']' is expected: %{}", t);
	}
    }
    return c7result_ok();
}


c7::result<>
proxy_array_base::dump_all(std::ostream& o, dump_context& dc, size_t n)
{
    std::string next_pref;
    std::string head_pref;

    o << '[';
    if (dc.indent) {
	dc.cur_indent += dc.indent;
	head_pref = "\n" + std::string(dc.cur_indent, ' ');
	next_pref = "," + head_pref;
    } else {
	head_pref = "";
	next_pref = ",";
    }

    const char *pref = head_pref.c_str();

    for (size_t i = 0; i < n; i++) {
	o << pref;
	if (auto res = dump_element(o, dc, i); !res) {
	    return res;
	}
	pref = next_pref.c_str();
    }

    if (dc.indent) {
	dc.cur_indent -= dc.indent;
	head_pref.resize(dc.cur_indent + 1);
	o << head_pref;
    }
    o << ']';

    return c7result_ok();
}


/*----------------------------------------------------------------------------
                               proxy_unconcern
----------------------------------------------------------------------------*/

c7::result<>
proxy_unconcern::load_array(lexer& lxr, token& t)
{
    store(t);
    if (lxr.get(t) != TKN_SQUARE_R) {
	for (;;) {
	    if (auto res = load_value(lxr, t); !res) {
		return res;
	    }
	    store(t);
	    if (lxr.get(t) != TKN_COMMA) {
		break;
	    }
	    store(t);
	    lxr.get(t);
	}
	if (t.code != TKN_SQUARE_R) {
	    return c7result_err(EINVAL, "']' is expected: %{}", t);
	}
    }
    return c7result_ok();
}

c7::result<>
proxy_unconcern::load_object(lexer& lxr, token& t)
{
    store(t);
    if (lxr.get(t) != TKN_CURLY_R) {
	for (;;) {
	    if (t.code != TKN_STRING) {
		return c7result_err(EINVAL, "keyword STRING is expceted: %{}", t);
	    }
	    store(t);
	    if (lxr.get(t) != TKN_COLON) {
		return c7result_err(EINVAL, "colon (:) is required: %{}", t);
	    }
	    store(t);
	    lxr.get(t);
	    if (auto res = load_value(lxr, t); !res) {
		return res;
	    }
	    store(t);
	    if (lxr.get(t) != TKN_COMMA) {
		break;
	    }
	    store(t);
	    lxr.get(t);
	}
	if (t.code != TKN_CURLY_R) {
	    return c7result_err(EINVAL, "'}' is expected: %{}", t);
	}
    }
    return c7result_ok();
}

c7::result<>
proxy_unconcern::load_value(lexer& lxr, token& t)
{
    if (t.code == TKN_TRUE || t.code == TKN_FALSE ||
	t.code == TKN_STRING ||
	t.code == TKN_INT || t.code == TKN_REAL) {
	return c7result_ok();
    }
    if (t.code == TKN_SQUARE_L) {
	return load_array(lxr, t);
    }
    if (t.code == TKN_CURLY_L) {
	return load_object(lxr, t);
    }
    return c7result_err(EINVAL, "It is not value: %{}", t);
}

void proxy_unconcern::store(token& t)
{
    tkns_.emplace_back(t.code, t.raw_str);
}

c7::result<>
proxy_unconcern::load(lexer& lxr, token& t)
{
    if (auto res = load_value(lxr, t); !res) {
	return res;
    }
    store(t);
    return c7result_ok();
}


c7::result<>
proxy_unconcern::dump(std::ostream& o, dump_context& dc)
{
    std::string pref;
    if (dc.indent) {
	pref = '\n' + std::string(dc.cur_indent, ' ');
    }

    for (auto& t: tkns_) {
	if (t.code == TKN_CURLY_L || t.code == TKN_SQUARE_L) {
	    o << t.raw_str;
	    if (dc.indent) {
		dc.cur_indent += dc.indent;
		pref.resize(dc.cur_indent + 1, ' ');
		o << pref;
	    }
	} else if (t.code == TKN_CURLY_R || t.code == TKN_SQUARE_R) {
	    if (dc.indent) {
		dc.cur_indent -= dc.indent;
		pref.resize(dc.cur_indent + 1);
		o << pref;
	    }
	    o << t.raw_str;
	} else if (t.code == TKN_COMMA) {
	    o << t.raw_str << pref;
	} else {
	    o << t.raw_str;
	}
    }
    return c7result_ok();
}


void proxy_unconcern::clear()
{
}


bool proxy_unconcern::check() const
{
    return true;
}


/*----------------------------------------------------------------------------
                                 proxy_object
----------------------------------------------------------------------------*/

c7::result<>
proxy_object::load(lexer& lxr, token& t)
{
    auto [ops, _] = proxy_attribute_();

    if (t.code != TKN_CURLY_L) {
	return c7result_err(EINVAL, "'{' is expected: %{}", t);
    }
    if (lxr.get(t) != TKN_CURLY_R) {
	for (;;) {
	    if (t.code != TKN_STRING) {
		return c7result_err(EINVAL, "keyword STRING is expceted: %{}", t);
	    }
	    auto key = t.str();
	    if (lxr.get(t) != TKN_COLON) {
		return c7result_err(EINVAL, "colon (:) is required: %{}", t);
	    }
	    lxr.get(t);
	    if (auto it = ops.find(key); it == ops.end()) {
		proxy_unconcern unc;
		if (auto res = unc.load(lxr, t); !res) {
		    return res;
		}
		unconcerns_.insert_or_assign(key, std::move(unc));
	    } else {
		auto& ops = (*it).second;
		if (auto res = ops.load(this, lxr, t); !res) {
		    return res;
		}
	    }
	    src_order_.push_back(key);
	    if (lxr.get(t) != TKN_COMMA) {
		break;
	    }
	    lxr.get(t);
	}
	if (t.code != TKN_CURLY_R) {
	    return c7result_err(EINVAL, "'}' is expected: %{}", t);
	}
    }
    return c7result_ok();
}


template <typename Object, typename... Args>
static c7::result<>
call_dump(std::unordered_map<std::string, Object>& map,
	  const std::string& key, Args&&... args)
{
    if (auto it = map.find(key); it == map.end()) {
	return c7result_err(ENOENT, "%{} is not found");
    } else {
	auto& obj = (*it).second;
	return obj.dump(std::forward<Args>(args)...);
    }
}

c7::result<>
proxy_object::dump(std::ostream& o, dump_context& dc)
{
    auto [ops, def_order] = proxy_attribute_();

    std::string next_pref;
    std::string head_pref;

    o << '{';
    if (dc.indent) {
	dc.cur_indent += dc.indent;
	head_pref = "\n" + std::string(dc.cur_indent, ' ');
	next_pref = "," + head_pref;
    } else {
	head_pref = "";
	next_pref = ",";
    }

    std::unordered_set<std::string> dumped;

    const char *pref = head_pref.c_str();

    for (auto& k: src_order_) {
	o << pref;
	o << '"' << k << "\":";
	if (auto res = call_dump(ops, k, this, o, dc); !res) {
	    if (!res.has_what(ENOENT)) {
		return res;
	    }
	    if (auto res = call_dump(unconcerns_, k, o, dc); !res) {
		return res;
	    }
	}
	dumped.insert(k);
	pref = next_pref.c_str();
    }

    for (auto& k: def_order) {
	if (dumped.find(k) != dumped.end()) {
	    continue;
	}
	o << pref;
	o << '"' << k << "\":";
	if (auto res = call_dump(ops, k, this, o, dc); !res) {
	    return res;
	}
	pref = next_pref.c_str();
    }

    if (dc.indent) {
	dc.cur_indent -= dc.indent;
	head_pref.resize(dc.cur_indent + 1);
	o << head_pref;
    }
    o << '}';

    return c7result_ok();
}


void
proxy_object::clear()
{
    auto [ops, _] = proxy_attribute_();

    for (auto& [k, ops]: ops) {
	ops.clear(this);
    }
}


bool
proxy_object::check() const
{
    auto [ops, _] = proxy_attribute_();

    for (auto& [k, ops]: ops) {
	if (!ops.check(this)) {
	    return false;
	}
    }
    return true;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::json
