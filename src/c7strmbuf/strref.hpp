/*
 * c7strmbuf/strref.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_STRMBUF_STRREF_HPP_LOADED__
#define C7_STRMBUF_STRREF_HPP_LOADED__
#include <c7common.hpp>


#include <cstring>
#include <memory>
#include <streambuf>
#include <string>


namespace c7::strmbuf {


class strref: public std::streambuf {
private:
    std::string& vb_;

protected:
    strref* setbuf(char_type* s, std::streamsize n) override {
	return this;
    }

    std::streamsize xsputn(const char_type *s, std::streamsize n) override;
    int_type overflow(int_type c = traits_type::eof()) override;

public:
    strref(std::string& vb): std::streambuf(), vb_(vb) {
	setp(vb_.data() + vb_.size(), vb_.data() + vb_.size());
    }

    virtual ~strref() {}

    size_t size() const {
	return vb_.size();
    }

    const char *data() const {
	return vb_.data();
    }

    char *data() {
	return vb_.data();
    }

    std::string& str() {
	return vb_;
    }

    const std::string& str() const {
	return vb_;
    }
};


} // namespace c7::strmbuf


#endif // c7strmbuf/strref.hpp
