/*
 * c7strmbuf/hybrid.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_STRMBUF_HYBRID_HPP_LOADED_
#define C7_STRMBUF_HYBRID_HPP_LOADED_
#include <c7common.hpp>


#include <cstring>
#include <memory>
#include <streambuf>
#include <c7utils/storage.hpp>


namespace c7::strmbuf {


class hybrid: public std::streambuf {
private:
    static constexpr size_t fb_size = 512;
    mutable char fb_[fb_size + 1];
    mutable c7::storage vb_{fb_size};		// align
    bool vb_available_ = false;

protected:
    hybrid* setbuf(char_type* s, std::streamsize n) override {
	return this;
    }

    std::streamsize xsputn(const char_type *s, std::streamsize n) override;
    int_type overflow(int_type c = traits_type::eof()) override;

public:
    hybrid(): std::streambuf() {
	setp(fb_, fb_ + fb_size);
    }

    virtual ~hybrid() {}

    size_t size() const {
	return (pptr() - pbase());
    }

    const char *data() const {
	*pptr() = 0;
	return pbase();
    }

    char *data() {
	*pptr() = 0;
	return pbase();
    }
};


} // namespace c7::strmbuf


#endif // c7strmbuf/hybrid.hpp
