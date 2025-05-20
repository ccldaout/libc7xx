/*
 * c7json/lexer.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_JSON_LEXER_HPP_LOADED_
#define C7_JSON_LEXER_HPP_LOADED_
#include <c7common.hpp>


#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <c7result.hpp>


namespace c7::json {


enum token_code {
    TKN_none,
    TKN_SQUARE_L,	// [ square bracket
    TKN_SQUARE_R,	// ]
    TKN_CURLY_L,	// {  curly bracket
    TKN_CURLY_R,	// }
    TKN_COLON,		// :
    TKN_COMMA,		// ,
    TKN_TRUE,		// true
    TKN_FALSE,		// false
    TKN_NULL,		// null
    TKN_STRING,
    TKN_INT,
    TKN_REAL,
    TKN_error,
};


struct token {
    token_code code;
    std::variant<int64_t, double, std::string> value;
    std::string raw_str;
    int n_line;
    int n_ch;

    token_code set(token_code tkc, int ch) {
	code = tkc;
	raw_str += ch;
	return tkc;
    }

    token_code set(token_code tkc, const std::string& s) {
	code = tkc;
	raw_str += s;
	return tkc;
    }

    std::string& str() {
	return std::get<std::string>(value);
    }

    int64_t i64() {
	return std::get<int64_t>(value);
    }

    double r64() {
	return std::get<double>(value);
    }

    c7::result<c7::usec_t> as_time();
};


class lexer {
public:
    lexer();
    ~lexer();

    c7::result<> start(std::istream& in);
    token_code get(token& tkn);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


void print_type(std::ostream&, const std::string&, token_code);
void print_type(std::ostream&, const std::string&, token);


} // namespace c7::json


#endif // c7json/lexer.hpp
