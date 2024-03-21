/*
 * c7nseq/urlencode.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_URLENCODE_HPP_LOADED__
#define C7_NSEQ_URLENCODE_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <cctype>


namespace c7::nseq {


struct urlencode_iter_end {};


template <typename Seq,
	  template <typename, typename> class ConvIter>
class urlconvert_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;

public:
    explicit urlconvert_seq(Seq seq):
	seq_(std::forward<Seq>(seq)) {
    }

    urlconvert_seq(const urlconvert_seq&) = delete;
    urlconvert_seq& operator=(const urlconvert_seq&) = delete;
    urlconvert_seq(urlconvert_seq&&) = default;
    urlconvert_seq& operator=(urlconvert_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return ConvIter<decltype(it), decltype(itend)>(it, itend);
    }

    auto end() {
	return urlencode_iter_end{};
    }

    auto begin() const {
	return const_cast<urlconvert_seq<Seq, ConvIter>*>(this)->begin();
    }

    auto end() const {
	return urlencode_iter_end{};
    }
};


/*----------------------------------------------------------------------------
                                urlencode encode
----------------------------------------------------------------------------*/

template <typename Iter, typename Iterend>
class urlencode_common_iter {
private:
    Iter it_;
    Iterend itend_;
    char cbuf_[3];
    int idx_;
    bool for_body_;

    void do_encode() {
	if (it_ == itend_) {
	    idx_ = -1;
	    return;
	}

	char c = *it_;
	++it_;

	if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
	    cbuf_[2] = c;
	    idx_ = 2;
	} else if (c == ' ' && for_body_) {
	    cbuf_[2] = '+';
	    idx_ = 2;
	} else {
	    static constexpr const char d[] = "0123456789ABCDEF";
	    cbuf_[0] = '%';
	    auto uc = static_cast<uint32_t>(c);
	    cbuf_[1] = d[uc >> 4];
	    cbuf_[2] = d[uc & 0xf];
	    idx_ = 0;
	}
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    urlencode_common_iter() = default;
    urlencode_common_iter(const urlencode_common_iter&) = default;
    urlencode_common_iter(urlencode_common_iter&&) = default;
    urlencode_common_iter& operator=(const urlencode_common_iter&) = default;
    urlencode_common_iter& operator=(urlencode_common_iter&&) = default;

    urlencode_common_iter(Iter it, Iterend itend, bool for_body):
	it_(it), itend_(itend), for_body_(for_body) {
	do_encode();
    }

    bool operator==(const urlencode_common_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator==(const urlencode_iter_end&) const {
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

    bool operator!=(const urlencode_common_iter& o) const {
	return !(*this == o);
    }

    bool operator!=(const urlencode_iter_end& o) const {
	return !(*this == o);
    }
};


template <typename Iter, typename Iterend>
class urlencode_url_iter: public urlencode_common_iter<Iter, Iterend> {
public:
    using urlencode_common_iter<Iter, Iterend>::urlencode_common_iter;

    urlencode_url_iter(Iter it, Iterend itend):
	urlencode_common_iter<Iter, Iterend>(it, itend, false) {
    }
};


template <typename Iter, typename Iterend>
class urlencode_body_iter: public urlencode_common_iter<Iter, Iterend> {
public:
    using urlencode_common_iter<Iter, Iterend>::urlencode_common_iter;

    urlencode_body_iter(Iter it, Iterend itend):
	urlencode_common_iter<Iter, Iterend>(it, itend, true) {
    }
};


class urlencode_url {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return urlconvert_seq<decltype(seq), urlencode_url_iter>(std::forward<Seq>(seq));
    }
};


class urlencode_body {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return urlconvert_seq<decltype(seq), urlencode_body_iter>(std::forward<Seq>(seq));
    }
};


/*----------------------------------------------------------------------------
                                urlencode decode
----------------------------------------------------------------------------*/


template <typename Iter, typename Iterend>
class urldecode_iter {
private:
    Iter it_;
    Iterend itend_;
    char ch_;
    bool end_ = false;

    char decode_ch(char t, char c) {
	static constexpr const char h[] = "0123456789ABCDEF";
	if (auto *p = std::strchr(h, std::toupper(c)); p != nullptr) {
	    return (t << 4) | (p - &h[0]);
	} else {
	    return t;
	}
    }

    void do_decode() {
	if (it_ == itend_) {
	    end_ = true;
	    return;
	}
	ch_ = *it_;
	++it_;
	if (ch_ == '%') {
	    if (it_ != itend_) {
		ch_ = decode_ch(0, *it_);
		++it_;
	    }
	    if (it_ != itend_) {
		ch_ = decode_ch(ch_, *it_);
		++it_;
	    }
	} else if (ch_ == '+') {
	    ch_ = ' ';
	}
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    urldecode_iter(Iter it, Iterend itend): it_(it), itend_(itend) {
	do_decode();
    }

    bool operator==(const urldecode_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator==(const urlencode_iter_end&) const {
	return end_;
    }

    auto& operator++() {
	do_decode();
	return *this;
    }

    uint8_t operator*() const {
	return ch_;
    }

    // deduced from aboves

    bool operator!=(const urldecode_iter& o) const {
	return !(*this == o);
    }

    bool operator!=(const urlencode_iter_end& o) const {
	return !(*this == o);
    }
};


class urldecode {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return urlconvert_seq<decltype(seq), urldecode_iter>(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::urlconvert_seq<Seq, c7::nseq::urlencode_url_iter>> {
    static constexpr const char *name = "urlencode[url]";
};
template <typename Seq>
struct format_ident<c7::nseq::urlconvert_seq<Seq, c7::nseq::urlencode_body_iter>> {
    static constexpr const char *name = "urlencode[body]";
};
template <typename Seq>
struct format_ident<c7::nseq::urlconvert_seq<Seq, c7::nseq::urldecode_iter>> {
    static constexpr const char *name = "urldecode";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/urlencode.hpp
