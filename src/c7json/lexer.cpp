/*
 * c7json/lexer.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <time.h>
#include <cctype>
#include <c7json/lexer.hpp>
#include <c7string/regex.hpp>
#include <c7string/utf8.hpp>


namespace c7::json {


/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

void print_type(std::ostream& out, const std::string&, token_code code)
{
#define def_(n)	case TKN_##n: out << #n; break
    switch (code) {
	def_(none);
	def_(SQUARE_L);
	def_(SQUARE_R);
	def_(CURLY_L);
	def_(CURLY_R);
	def_(COLON);
	def_(COMMA);
	def_(TRUE);
	def_(FALSE);
	def_(NULL);
	def_(STRING);
	def_(INT);
	def_(REAL);
	def_(error);
    }
#undef def_
}


void print_type(std::ostream& out, const std::string&, token tkn)
{
    c7::format(out, "token<%{}, raw_str<%{}>", tkn.code, tkn.raw_str);
    if (tkn.code == TKN_STRING) {
	c7::format(out, ", value:<%{}>>", tkn.str());
    } else if (tkn.code == TKN_INT) {
	c7::format(out, ", value:<%{}>>", tkn.i64());
    } else if (tkn.code == TKN_REAL) {
	c7::format(out, ", value:<%{}>>", tkn.r64());
    } else {
	out << ">";
    }
}


/*----------------------------------------------------------------------------
                                    token
----------------------------------------------------------------------------*/

c7::result<c7::usec_t>
token::as_time()
{
    static struct gmtoff {
	gmtoff() {
	    time_t tv = time(nullptr);
	    auto tm = localtime(&tv);
	    tm_gmtoff = tm->tm_gmtoff;
	}
	int64_t tm_gmtoff;
    } tz;

    static c7::str::regex reg{R"--("(\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d)(\.(\d\d\d(\d\d\d)?))?([-+]\d\d):?(\d\d)")--"};
    c7::strvec sub;

    if (reg.match(raw_str, sub)) {
	auto ts = sub[1] + sub[5] + sub[6];
	struct tm tmbuf{};
	if (::strptime(ts.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tmbuf) != nullptr) {
	    tmbuf.tm_isdst = 0;
	    // adjust manually hour (and minute) for timezone offset,
	    // because mktime() ignore tm_gmtoff (timezone is no concerted).
	    auto off_s  = tz.tm_gmtoff - tmbuf.tm_gmtoff;
	    auto off_h  =  off_s / 3600;
	    auto off_m  = (off_s % 3600) / 60;
	    tmbuf.tm_hour += off_h;
	    tmbuf.tm_min  += off_m;
	    c7::usec_t tv = ::mktime(&tmbuf);
	    tv *= C7_TIME_S_us;
	    if (!sub[3].empty()) {
		uint64_t fact = sub[4].empty() ? 1000 : 1;
		tv += (std::stoul(sub[3]) * fact);
	    }
	    return c7result_ok(tv);
	}
	return c7result_err(EINVAL, "strptime failed");
    }
    return c7result_err(EINVAL);
}


/*----------------------------------------------------------------------------
                               lexer::impl
----------------------------------------------------------------------------*/

class lexer::impl {
public:
    explicit impl(std::istream& in): in_(in) {}
    token_code get(token& tkn);

private:
    std::istream& in_;
    std::string raw_;
    int c_;

    bool eof() { return in_.eof(); }
    int get();
    void unget();
    int skip_spaces();
    token_code expect_keyword(token&, token_code, const char *word);
    token_code expect_string(token&);
    token_code expect_number(token&);
};


int
lexer::impl::get()
{
    c_ = in_.get();
    if (!in_.eof()) {
	raw_ += c_;
    }
    return c_;
}


void
lexer::impl::unget()
{
    raw_.pop_back();
    in_.unget();
}


int
lexer::impl::skip_spaces()
{
    for (;;) {
	switch (get()) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	    continue;
	default:
	    raw_ = c_;
	    return c_;
	}
    }
}


token_code
lexer::impl::get(token& tkn)
{
    tkn.code = TKN_error;
    tkn.raw_str.clear();

    skip_spaces();
    if (eof()) {
	tkn.code = TKN_none;
	return tkn.code;
    }

    switch (c_) {
    case '[':
	return tkn.set(TKN_SQUARE_L, c_);
    case ']':
	return tkn.set(TKN_SQUARE_R, c_);
    case '{':
	return tkn.set(TKN_CURLY_L, c_);
    case '}':
	return tkn.set(TKN_CURLY_R, c_);
    case ':':
	return tkn.set(TKN_COLON, c_);
    case ',':
	return tkn.set(TKN_COMMA, c_);

    case 't':
	return expect_keyword(tkn, TKN_TRUE, "true");
    case 'f':
	return expect_keyword(tkn, TKN_FALSE, "false");
    case 'n':
	return expect_keyword(tkn, TKN_NULL, "null");

    case '"':
	return expect_string(tkn);

    default:
	if (std::isdigit(c_) || c_ == '-') {
	    return expect_number(tkn);
	} else {
	    return tkn.set(TKN_error, c_);
	}
    }
}


token_code
lexer::impl::expect_keyword(token& tkn, token_code code, const char *word)
{
    for (++word; *word; word++) {
	if (get() != *word) {
	    return tkn.set(TKN_error, raw_);
	}
    }
    return tkn.set(code, raw_);
}


token_code
lexer::impl::expect_string(token& tkn)
{
    std::string eval;
    for (;;) {
	if (get() == '"') {
	    break;
	}
	if (c_ == '\\') {
	    switch (get()) {
	    case 'b':	eval += '\b';	break;
	    case 'f':	eval += '\f';	break;
	    case 'n':	eval += '\n';	break;
	    case 'r':	eval += '\r';	break;
	    case 't':	eval += '\t';	break;

	    case '\\':
	    case '"':
	    case '/':	eval += c_;	break;

	    case 'u': {
		std::string hex;
		for (int i = 0; i < 4; i++) {
		    if (!std::isxdigit(get())) {
			return tkn.set(TKN_error, raw_);
		    }
		    hex += c_;
		}
		uint32_t u = std::stoul(hex, nullptr, 16);
		if (!c7::utf8::append_from_utf32(u, eval)) {
		    return tkn.set(TKN_error, raw_);
		}
	    }				break;

	    default:
		return tkn.set(TKN_error, raw_);
	    }
	} else {
	    eval += c_;
	}
    }

    tkn.value = eval;
    return tkn.set(TKN_STRING, raw_);
}


token_code
lexer::impl::expect_number(token& tkn)
{
    bool is_float = false;
    if (c_ == '-') {
	get();
    }
    if (c_ == '0') {
	get();
    } else if (std::isdigit(c_)) {
	while (std::isdigit(get())) {}
    } else {
	return tkn.set(TKN_error, raw_);
    }
    if (c_ == '.') {
	is_float = true;
	int n = 0;
	while (std::isdigit(get())) { n++; }
	if (n == 0) {
	    return tkn.set(TKN_error, raw_);
	}
    }
    if (c_ == 'e') {
	is_float = true;
	get();
	if (c_ == '+' || c_ == '-') {
	    get();
	}
	int n = 0;
	while (std::isdigit(c_)) { get(); n++; }
	if (n == 0) {
	    return tkn.set(TKN_error, raw_);
	}
    }
    unget();

    if (is_float) {
	tkn.value = std::stod(raw_);
	return tkn.set(TKN_REAL, raw_);
    } else {
	tkn.value = std::stol(raw_);
	return tkn.set(TKN_INT, raw_);
    }
}


/*----------------------------------------------------------------------------
                                  lexer
----------------------------------------------------------------------------*/

lexer::lexer() = default;

lexer::~lexer() = default;

c7::result<>
lexer::start(std::istream& in)
{
    pimpl_.reset(new impl(in));
    return c7result_ok();
}

token_code
lexer::get(token& tkn)
{
    return pimpl_->get(tkn);
}


} // namespace c7::json
