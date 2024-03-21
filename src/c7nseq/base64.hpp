/*
 * c7nseq/base64.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_BASE64_HPP_LOADED__
#define C7_NSEQ_BASE64_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct base64_iter_end {};


template <typename Seq,
	  template <typename, typename> class ConvIter>
class base64cnv_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;

public:
    explicit base64cnv_seq(Seq seq):
	seq_(std::forward<Seq>(seq)) {
    }

    base64cnv_seq(const base64cnv_seq&) = delete;
    base64cnv_seq& operator=(const base64cnv_seq&) = delete;
    base64cnv_seq(base64cnv_seq&&) = default;
    base64cnv_seq& operator=(base64cnv_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return ConvIter<decltype(it), decltype(itend)>(it, itend);
    }

    auto end() {
	return base64_iter_end{};
    }

    auto begin() const {
	return const_cast<base64cnv_seq<Seq, ConvIter>*>(this)->begin();
    }

    auto end() const {
	return base64_iter_end{};
    }
};


/*----------------------------------------------------------------------------
                                base64 encode
----------------------------------------------------------------------------*/

template <typename Iter, typename Iterend>
class base64enc_iter {
private:
    static constexpr char enctab_[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
    };

    Iter it_;
    Iterend itend_;
    char cbuf_[4];
    int idx_;

    void do_encode() {
	if (it_ == itend_) {
	    idx_ = -1;
	    return;
	}

	uint8_t ubuf[3];
	int n = 0;
	ubuf[n++] = *it_;
	if (++it_ != itend_) {
	    ubuf[n++] = *it_;
	    if (++it_ != itend_) {
		ubuf[n++] = *it_;
		++it_;
	    } else {
		ubuf[2] = 0;
	    }
	} else {
	    ubuf[1] = ubuf[2] = 0;
	}

	cbuf_[0] = enctab_[                          ubuf[0] >> 2 ];
	cbuf_[1] = enctab_[((ubuf[0] & 0x3) << 4) | (ubuf[1] >> 4)];
	cbuf_[2] = enctab_[((ubuf[1] & 0xf) << 2) | (ubuf[2] >> 6)];
	cbuf_[3] = enctab_[  ubuf[2] & 0x3f                       ];

	if (n != 3) {
	    cbuf_[3] = '=';
	    if (n == 1) {
		cbuf_[2] = '=';
	    }
	}

	idx_ = 0;
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    base64enc_iter() = default;
    base64enc_iter(const base64enc_iter&) = default;
    base64enc_iter(base64enc_iter&&) = default;
    base64enc_iter& operator=(const base64enc_iter&) = default;
    base64enc_iter& operator=(base64enc_iter&&) = default;

    base64enc_iter(Iter it, Iterend itend): it_(it), itend_(itend) {
	do_encode();
    }

    bool operator==(const base64enc_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator==(const base64_iter_end&) const {
	return (idx_ == -1);
    }

    auto& operator++() {
	idx_++;
	if (idx_ == c7_numberof(cbuf_)) {
	    do_encode();
	}
	return *this;
    }

    char operator*() const {
	return cbuf_[idx_];
    }

    // deduced from aboves

    bool operator!=(const base64enc_iter& o) const {
	return !(*this == o);
    }

    bool operator!=(const base64_iter_end& o) const {
	return !(*this == o);
    }
};


class base64enc {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return base64cnv_seq<decltype(seq), base64enc_iter>(std::forward<Seq>(seq));
    }
};


/*----------------------------------------------------------------------------
                                base64 decode
----------------------------------------------------------------------------*/


template <typename Iter, typename Iterend>
class base64dec_iter {
private:
    static constexpr uint8_t dectab_[] = {
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, 62, -0, -0, -0, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -0, -0, -0, -0, -0, -0,
	-0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -0, -0, -0, -0, -0,
	-0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0,
	-0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0, -0
    };

    Iter it_;
    Iterend itend_;
    uint8_t ubuf_[3];
    int idx_;

    void do_decode() {
	if (it_ == itend_) {
	    idx_ = -1;
	    return;
	}

	int i = 0;
	idx_ = 0;

	uint8_t u[4];
	u[0] = dectab_[static_cast<int>(*it_)];
	++it_;
	u[1] = dectab_[static_cast<int>(*it_)];

	if (it_ != itend_) {
	    ++it_;
	}
	u[2] = dectab_[static_cast<int>(*it_)];
	if (*it_ == '=') {
	    i = idx_ = 2;
	}

	if (it_ != itend_) {
	    ++it_;
	}
	u[3] = dectab_[static_cast<int>(*it_)];
	if (*it_ == '=' && idx_ == 0) {
	    i = idx_ = 1;
	}

	if (it_ != itend_) {
	    ++it_;
	}

	ubuf_[i++] = (u[0] << 2) | (u[1] >> 4);
	if (i != 3) {
	    ubuf_[i++] = ((u[1] & 0xf) << 4) | (u[2] >> 2);
	    if (i != 3) {
		ubuf_[i++] = ((u[2] & 0x3) << 6) | u[3];
	    }
	}
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    base64dec_iter() = default;
    base64dec_iter(const base64dec_iter&) = default;
    base64dec_iter(base64dec_iter&&) = default;
    base64dec_iter& operator=(const base64dec_iter&) = default;
    base64dec_iter& operator=(base64dec_iter&&) = default;

    base64dec_iter(Iter it, Iterend itend): it_(it), itend_(itend) {
	do_decode();
    }

    bool operator==(const base64dec_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator==(const base64_iter_end&) const {
	return (idx_ == -1);
    }

    auto& operator++() {
	idx_++;
	if (idx_ == c7_numberof(ubuf_)) {
	    do_decode();
	}
	return *this;
    }

    uint8_t operator*() const {
	return ubuf_[idx_];
    }

    // deduced from aboves

    bool operator!=(const base64dec_iter& o) const {
	return !(*this == o);
    }

    bool operator!=(const base64_iter_end& o) const {
	return !(*this == o);
    }
};


class base64dec {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return base64cnv_seq<decltype(seq), base64dec_iter>(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::base64cnv_seq<Seq, c7::nseq::base64enc_iter>> {
    static constexpr const char *name = "base64enc";
};
template <typename Seq>
struct format_ident<c7::nseq::base64cnv_seq<Seq, c7::nseq::base64dec_iter>> {
    static constexpr const char *name = "base64dec";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/base64.hpp
