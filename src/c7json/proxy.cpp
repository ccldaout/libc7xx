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
                               helper functions
----------------------------------------------------------------------------*/

void jsonize_string(std::ostream& o, const std::string& s)
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


dump_helper::dump_helper(): head_("\n"), next_(",\n") {}


dump_helper::dump_helper(dump_context dc): context(dc), head_("\n"), next_(",\n") {}


void
dump_helper::begin(std::ostream& o, char c)
{
    o << c;
    if (context.indent) {
	context.cur_indent += context.indent;
	head_.resize(context.cur_indent + 1, ' ');
	next_.resize(context.cur_indent + 2, ' ');
    } else {
	head_ = "";
	next_ = ",";
    }
    pref_ = head_.c_str();
}


const char *
dump_helper::pref()
{
    auto pref = pref_;
    pref_ = next_.c_str();
    return pref;
}


void
dump_helper::end(std::ostream& o, char c)
{
    if (context.indent) {
	context.cur_indent -= context.indent;
	head_.resize(context.cur_indent + 1);
	next_.resize(context.cur_indent + 2, ' ');
	o << head_;
    }
    pref_ = next_.c_str();
    o << c;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

// integer

template <> c7::result<>
proxy_basic_strict<int64_t>::load_impl(lexer&, token& tkn)
{
    if (tkn.code == TKN_INT) {
	val_ = tkn.i64();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "Integer is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic_strict<int64_t>::dump_impl(std::ostream& o, dump_context&) const
{
    o << std::dec << val_;
    return c7result_ok();
}


// real number

template <> c7::result<>
proxy_basic_strict<double>::load_impl(lexer&, token& tkn)
{
    if (tkn.code == TKN_REAL) {
	val_ = tkn.r64();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "Real number is expected: %{}", tkn);
}

template <> c7::result<>
proxy_basic_strict<double>::dump_impl(std::ostream& o, dump_context&) const
{
    o << std::defaultfloat << val_;
    return c7result_ok();
}


// bool

template <> c7::result<>
proxy_basic_strict<bool>::load_impl(lexer&, token& tkn)
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
proxy_basic_strict<bool>::dump_impl(std::ostream& o, dump_context&) const
{
    o << std::boolalpha << val_;
    return c7result_ok();
}


// string

template <> c7::result<>
proxy_basic_strict<std::string>::load_impl(lexer&, token& tkn)
{
    if (tkn.code == TKN_STRING) {
	val_ = tkn.str();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "String \"...\" is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic_strict<std::string>::dump_impl(std::ostream& o, dump_context&) const
{
    jsonize_string(o, val_);
    return c7result_ok();
}


template <> c7::result<>
proxy_basic_strict<std::string>::from_str(token& tkn)
{
    val_ = tkn.str();
    return c7result_ok();
}


// time_us

template <> c7::result<>
proxy_basic_strict<time_us>::load_impl(lexer&, token& tkn)
{
    if (auto res = tkn.as_time(); res) {
	val_ = res.value();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "ISO8601 Date and time string is expected: %{}", tkn);
}


template <> c7::result<>
proxy_basic_strict<time_us>::dump_impl(std::ostream& o, dump_context& dc) const
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


template <> c7::result<>
proxy_basic_strict<time_us>::from_str(token& tkn)
{
    if (auto res = tkn.as_time(); res) {
	val_ = res.value();
	return c7result_ok();
    }
    return c7result_err(EINVAL, "ISO8601 Date and time string is expected: %{}", tkn);
}


// binary_t (std::vector<uint8_t>)

template <> c7::result<>
proxy_basic_strict<binary_t>::load_impl(lexer&, token& tkn)
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
proxy_basic_strict<binary_t>::dump_impl(std::ostream& o, dump_context&) const
{
    binary_t empty;
    std::string s;
    val_ | c7::nseq::base64enc() | c7::nseq::push_back(s);
    jsonize_string(o, s);
    return c7result_ok();
}


template <> c7::result<>
proxy_basic_strict<binary_t>::from_str(token& tkn)
{
    binary_t ba;
    tkn.str() | c7::nseq::base64dec() | c7::nseq::push_back(ba);
    val_ = std::move(ba);
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                  proxy_pair
----------------------------------------------------------------------------*/

c7::result<>
proxy_pair_base::load_impl(lexer& lxr, token& t, load_func load_elements)
{
    if (t.code != TKN_SQUARE_L) {
	return c7result_err(EINVAL, "'[' is expected: %{}", t);
    }
    lxr.get(t);
    if (auto res = load_elements(lxr, t); !res) {
	return res;
    }
    if (lxr.get(t) != TKN_SQUARE_R) {
	return c7result_err(EINVAL, "']' is expected: %{}", t);
    }
    return c7result_ok();
}


c7::result<>
proxy_pair_base::dump_impl(std::ostream& o, dump_helper& dh, dump_func dump_elements) const
{
    dh.begin(o, '[');

    o << dh.pref();
    if (auto res = dump_elements(o, dh); !res) {
	return res;
    }

    dh.end(o, ']');
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                 proxy_tuple
----------------------------------------------------------------------------*/

c7::result<>
proxy_tuple_base::load_impl(lexer& lxr, token& t, load_func load_elements)
{
    if (t.code != TKN_SQUARE_L) {
	return c7result_err(EINVAL, "'[' is expected: %{}", t);
    }
    if (lxr.get(t) != TKN_SQUARE_R) {
	if (auto res = load_elements(lxr, t); !res) {
	    return res;
	}
	if (t.code != TKN_SQUARE_R) {
	    return c7result_err(EINVAL, "']' is expected: %{}", t);
	}
    }
    return c7result_ok();
}


c7::result<>
proxy_tuple_base::dump_impl(std::ostream& o, dump_helper& dh, dump_func dump_elements) const
{
    dh.begin(o, '[');

    if (auto res = dump_elements(o, dh); !res) {
	return res;
    }

    dh.end(o, ']');
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
proxy_array_base::dump_all(std::ostream& o, dump_helper& dh, size_t n) const
{
    dh.begin(o, '[');

    for (size_t i = 0; i < n; i++) {
	o << dh.pref();
	if (auto res = dump_element(o, dh, i); !res) {
	    return res;
	}
    }

    dh.end(o, ']');
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                  proxy_dict
----------------------------------------------------------------------------*/

c7::result<>
proxy_dict_base::load(lexer& lxr, token& t)
{
    if (t.code != TKN_CURLY_L) {
	return c7result_err(EINVAL, "'{' is expected: %{}", t);
    }
    if (lxr.get(t) != TKN_CURLY_R) {
	for (;;) {
	    if (t.code != TKN_STRING) {
		return c7result_err(EINVAL, "keyword STRING is expceted: %{}", t);
	    }
	    auto key = t;
	    if (lxr.get(t) != TKN_COLON) {
		return c7result_err(EINVAL, "colon (:) is required: %{}", t);
	    }
	    lxr.get(t);
	    if (auto res = load_element(lxr, t, key); !res) {
		return res;
	    }
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


c7::result<>
proxy_dict_base::dump(std::ostream& o, dump_helper& dh) const
{
    dh.begin(o, '{');

    if (dump_start()) {
	do {
	    o << dh.pref();
	    if (auto res = dump_element(o, dh); !res) {
		return res;
	    }
	} while (dump_next());
    }

    dh.end(o, '}');
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
    if (save_) {
	tkns_.emplace_back(t.code, t.raw_str);
    }
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
proxy_unconcern::dump(std::ostream& o, dump_helper& dh) const
{
    for (auto& t: tkns_) {
	if (t.code == TKN_CURLY_L || t.code == TKN_SQUARE_L) {
	    dh.begin(o, t.raw_str[0]);
	    o << dh.pref();
	} else if (t.code == TKN_CURLY_R || t.code == TKN_SQUARE_R) {
	    dh.end(o, t.raw_str[0]);
	} else if (t.code == TKN_COMMA) {
	    o << t.raw_str << dh.pref();
	} else {
	    o << t.raw_str;
	}
    }
    return c7result_ok();
}


void proxy_unconcern::clear()
{
}


/*----------------------------------------------------------------------------
                  proxy_struct / proxy_strict / proxy_object
----------------------------------------------------------------------------*/

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


template <typename Derived>
c7::result<>
proxy_struct_base<Derived>::load_impl(lexer& lxr, token& t, attr_t attr)
{
    auto [ops_dic, _] = attr;

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

	    if (auto it = ops_dic.find(key); it != ops_dic.end()) {
		auto& ops = (*it).second;
		if (auto res = ops.load(this, lxr, t); !res) {
		    return res;
		}
	    } else {
		auto derived = static_cast<Derived*>(this);
		if (auto res = derived->load_custom(lxr, t, key); !res) {
		    return res;
		}
	    }

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


template <typename Derived>
c7::result<>
proxy_struct_base<Derived>::dump_impl(std::ostream& o, dump_helper& dh, attr_t attr) const
{
    dh.begin(o, '{');

    auto [ops_dic, def_order] = attr;

    for (auto& k: def_order) {
	o << dh.pref();
	o << '"' << k << "\":";
	if (auto res = call_dump(ops_dic, k, this, o, dh); !res) {
	    return res;
	}
    }

    auto derived = static_cast<const Derived*>(this);
    if (auto res = derived->dump_custom(o, dh); !res) {
	return res;
    }

    dh.end(o, '}');
    return c7result_ok();
}


template <typename Derived>
void
proxy_struct_base<Derived>::clear_impl(attr_t attr)
{
    auto [ops_dic, _] = attr;
    for (auto& [k, ops]: ops_dic) {
	ops.clear(this);
    }
    static_cast<Derived*>(this)->clear_custom();
}


// proxy_struct

c7::result<>
proxy_struct::load_custom(lexer& lxr, token& t, const std::string& unknown_key)
{
    proxy_unconcern unc{false};
    return unc.load(lxr, t);
}


// proxy_strict

c7::result<>
proxy_strict::load_custom(lexer& lxr, token& t, const std::string& unknown_key)
{
    return c7result_err(EINVAL, "unknown member: \"%{}\", value:%{}", unknown_key, t);
}


// proxy_object

c7::result<>
proxy_object::load_custom(lexer& lxr, token& t, const std::string& unknown_key)
{
    proxy_unconcern unc;
    if (auto res = unc.load(lxr, t); !res) {
	return res;
    }
    if (!unconcerns_) {
	unconcerns_ = std::make_unique<decltype(unconcerns_)::element_type>();
    }
    unconcerns_->insert_or_assign(unknown_key, std::move(unc));
    return c7result_ok();
}


c7::result<>
proxy_object::dump_custom(std::ostream& o, dump_helper& dh) const
{
    if (unconcerns_) {
	for (auto& [k, unc]: *unconcerns_) {
	    o << dh.pref();
	    o << '"' << k << "\":";
	    if (auto res = unc.dump(o, dh); !res) {
		return res;
	    }
	}
    }
    return c7result_ok();
}


void
proxy_object::clear_custom()
{
    if (unconcerns_) {
	unconcerns_->clear();
    }
}


// explicit instantiation

template class proxy_struct_base<proxy_struct>;
template class proxy_struct_base<proxy_strict>;
template class proxy_struct_base<proxy_object>;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::json
