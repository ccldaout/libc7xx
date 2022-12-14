/*
 * c7seq.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=637771050
 */
#ifndef C7_SEQ_HPP_LOADED__
#define C7_SEQ_HPP_LOADED__
#include <c7common.hpp>


#include <cstdio>
#include <functional>
#include <iterator>	// std::iterator_traits
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <c7utils.hpp>	// c_array_iterator


namespace c7::seq {


/*----------------------------------------------------------------------------
                                    range
----------------------------------------------------------------------------*/

template <typename T>
class range_seq {
private:
    typedef typename std::remove_const<T>::type V;

    const size_t n_;
    const V init_, step_;

public:
    class iterator {
	const range_seq<T>& seq_;
	size_t idx_;
	V val_;

	V calc() {
	    return seq_.init_ + seq_.step_ * idx_;
	};

    public:
	typedef ptrdiff_t difference_type;
	typedef V value_type;
	typedef V* pointer;
	typedef V& reference;
	typedef std::input_iterator_tag iterator_category;

	iterator(const range_seq<T>& seq, size_t idx):
	    seq_(seq), idx_(idx) {
	    val_ = calc();
	}

	bool operator==(const iterator& rhs) const {
	    return idx_ == rhs.idx_;
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (idx_ < seq_.n_) {
		idx_++;
		if (std::is_floating_point_v<T>)
		    val_ = calc();
		else
		    val_ += seq_.step_;
	    }
	    return *this;
	}

	value_type operator*() {
	    if (idx_ >= seq_.n_)
		throw std::out_of_range("Maybe end iterator.");
	    return val_;
	}
    };

    typedef iterator const_iterator;

    explicit range_seq(size_t n, T init=0, T step=1):
	n_(n), init_(init), step_(step) {
    }

    iterator begin() noexcept {
	return iterator(*this, 0);
    }
    
    iterator end() noexcept {
	return iterator(*this, n_);
    }
};

template <typename T>
range_seq<T> range(size_t n, T init=T(0), T step=T(1))
{
    return range_seq<T>(n, init, step);
}


/*----------------------------------------------------------------------------
                                     head
----------------------------------------------------------------------------*/

template <typename C>
class head_seq {
private:
    typedef std::remove_reference_t<C> C_;
    typedef typename std::conditional_t<std::is_rvalue_reference_v<C>, C_, C_&> S;
    typedef typename std::conditional_t<std::is_rvalue_reference_v<C>, C_&&, C_&> A;

    S c;
    size_t len;

public:
    class iterator {
    private:
	typedef decltype(std::declval<C>().begin()) item_iterator;
	item_iterator it_;
	size_t idx_;
	bool terminator_;

    public:
	typedef ptrdiff_t difference_type;
	typedef typename std::iterator_traits<item_iterator>::value_type value_type;
	typedef typename std::iterator_traits<item_iterator>::pointer pointer;
	typedef typename std::iterator_traits<item_iterator>::reference reference;
	typedef std::input_iterator_tag iterator_category;

	iterator(C_& c, size_t idx):
	    it_(c.begin()), idx_(idx), terminator_(idx != 0) {
	}

	bool operator==(const iterator& rhs) const {
	    return idx_ == rhs.idx_;
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (terminator_)
		throw std::runtime_error("end iterator refuse operator++");
	    ++it_;
	    ++idx_;
	    return *this;
	}

	decltype(*it_) operator*() {
	    if (terminator_)
		throw std::out_of_range("Maybe end iterator.");
	    return *it_;
	}
    };

    typedef iterator const_iterator;

    head_seq(A c, size_t len): c(std::forward<C>(c)), len(len) {
    }

    head_seq(head_seq<C>&& h): c(std::forward<C>(h.c)), len(h.len) {
    }

    head_seq(head_seq<C>& h): c(h.c), len(h.len) {
    }

    head_seq(const head_seq<C>& h): c(h.c), len(h.len) {
    }

    iterator begin() noexcept {
	return iterator(c, 0);
    }

    iterator end() noexcept {
	return iterator(c, len);
    }
};

template <typename C>
head_seq<C&&> head(C&& c, size_t len)
{
    return head_seq<C&&>(std::forward<C>(c), len);
}


/*----------------------------------------------------------------------------
                                     tail
----------------------------------------------------------------------------*/

template <typename C>
class tail_seq {
public:
    typedef decltype(std::declval<C>().begin()) iterator;
    typedef iterator const_iterator;

private:
    typedef std::remove_reference_t<C> C_;
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_, C_&>::type S;
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_&&, C_&>::type A;

    S c;
    size_t off;

    iterator begin_impl(std::input_iterator_tag) noexcept {
	auto it = c.begin();
	while (off-- > 0)
	    ++it;
	return it;
    }
    iterator begin_impl(std::random_access_iterator_tag) noexcept {
	return c.begin() + off;
    }

public:
    tail_seq(A c, size_t off): c(std::forward<C>(c)), off(off) {
    }

    tail_seq(tail_seq<C>&& h): c(std::forward<C>(h.c)), off(h.off) {
    }

    tail_seq(tail_seq<C>& h): c(h.c), off(h.off) {
    }

    tail_seq(const tail_seq<C>& h): c(h.c), off(h.off) {
    }

    iterator begin() noexcept {
	return begin_impl(typename std::iterator_traits<iterator>::iterator_category());
    }

    iterator end() noexcept {
	return c.end();
    }
};

template <typename C>
tail_seq<C&&> tail(C&& c, size_t len)
{
    return tail_seq<C&&>(std::forward<C>(c), len);
}


/*----------------------------------------------------------------------------
                                   reverse
----------------------------------------------------------------------------*/

template <typename C>
class reverse_seq {
private:
    typedef typename std::remove_reference_t<C> C_;
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_, C_&>::type S;
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_&&, C_&>::type A;

    S c;

public:
    typedef decltype(c.rbegin()) iterator;
    typedef iterator const_iterator;

    explicit reverse_seq(A c): c(std::forward<C>(c)) {
    }

    reverse_seq(reverse_seq<C>&& h): c(std::forward<C>(h.c)) {
    }

    reverse_seq(reverse_seq<C>& h): c(h.c) {
    }

    reverse_seq(const reverse_seq<C>& h): c(h.c) {
    }

    iterator begin() noexcept {
	return c.rbegin();
    }

    iterator end() noexcept {
	return c.rend();
    }
};

template <typename C>
reverse_seq<C&&> reverse(C&& c)
{
    return reverse_seq<C&&>(std::forward<C>(c));
}


/*----------------------------------------------------------------------------
                                  enumerate
----------------------------------------------------------------------------*/

template <typename C>
class enumerate_seq {
private:
    typedef typename std::remove_reference_t<C> C_;
    typedef typename std::conditional_t<std::is_rvalue_reference_v<C>, C_, C_&> S;
    typedef typename std::conditional_t<std::is_rvalue_reference_v<C>, C_&&, C_&> A;

    S c;
    ssize_t start_idx;

public:
    class iterator {
    private:
	typedef decltype(std::declval<C>().begin()) item_iterator;
	item_iterator it_;
	ssize_t idx_;

    public:
	typedef ptrdiff_t difference_type;
	typedef std::pair<ssize_t, decltype(*it_)> value_type;
	typedef value_type* pointer;
	typedef value_type& reference;
	typedef std::input_iterator_tag iterator_category;

	iterator(C_& c):
	    it_(c.end()) {
	}

	iterator(C_& c, ssize_t idx):
	    it_(c.begin()), idx_(idx) {
	}

	bool operator==(const iterator& rhs) const {
	    return it_ == rhs.it_;
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    ++it_;
	    ++idx_;
	    return *this;
	}

	value_type operator*() {
	    return { idx_, *it_ };
	}
    };

    typedef iterator const_iterator;

public:
    explicit enumerate_seq(A c, ssize_t index = 0):
	c(std::forward<C>(c)), start_idx(index) {
    }

    enumerate_seq(enumerate_seq<C>&& h): c(std::forward<C>(h.c)), start_idx(h.start_idx) {
    }

    enumerate_seq(enumerate_seq<C>& h): c(h.c), start_idx(h.start_idx) {
    }

    enumerate_seq(const enumerate_seq<C>& h): c(h.c), start_idx(h.start_idx) {
    }

    iterator begin() noexcept {
	return iterator(c, start_idx);
    }

    iterator end() noexcept {
	return iterator(c);
    }
};

template <typename C>
enumerate_seq<C&&> enumerate(C&& c, ssize_t index = 0)
{
    return enumerate_seq<C&&>(std::forward<C>(c), index);
}


/*----------------------------------------------------------------------------
                              convert / pointer
----------------------------------------------------------------------------*/

template <typename C, typename F>
class convert_seq {
private:
    using C_ = std::remove_reference_t<C>;

public:
    using item_iterator	 = decltype(std::declval<C>().begin());
    using item_reference = typename std::iterator_traits<item_iterator>::value_type&;
    using item_converter = F;

    class iterator {
    private:
	item_iterator	it_;
	item_converter&	conv_;

    public:
	using difference_type	= ptrdiff_t;
	using value_type	= typename std::invoke_result_t<F, item_reference>;
	using pointer		= value_type*;
	using reference		= value_type&;
	using iterator_category	= std::input_iterator_tag;

	iterator(C_& c, item_converter& conv):
	    it_(c.begin()), conv_(conv) {
	}

	iterator(C_& c, item_converter& conv, bool):
	    it_(c.end()), conv_(conv) {
	}

	bool operator==(const iterator& rhs) const {
	    return (it_ == rhs.it_);
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    ++it_;
	    return *this;
	}

	value_type operator*() {
	    return conv_(*it_);
	}
    };

    using const_iterator = iterator;

private:
    using S = typename std::conditional_t<std::is_rvalue_reference_v<C>, C_, C_&>;
    using A = typename std::conditional_t<std::is_rvalue_reference_v<C>, C_&&, C_&>;

    S c;
    item_converter conv;

public:
    convert_seq(A c, item_converter conv):
	c(std::forward<C>(c)), conv(conv) {
    }

    convert_seq(convert_seq<C, F>&& h): c(std::forward<C>(h.c)), conv(h.conv) {
    }

    convert_seq(convert_seq<C, F>& h): c(h.c), conv(h.conv) {
    }

    convert_seq(const convert_seq<C, F>& h): c(h.c), conv(h.conv) {
    }

    iterator begin() noexcept {
	return iterator(c, conv);
    }

    iterator end() noexcept {
	return iterator(c, conv, false);
    }
};

template <typename C, typename F>
convert_seq<C&&, F> convert(C&& c, F conv)
{
    return convert_seq<C&&, F>(std::forward<C>(c), conv);
}

template <typename C>
auto ptr(C&& c)
{
    using ref_type =
	typename std::iterator_traits<decltype(c.begin())>::reference;
    return c7::seq::convert(std::forward<C>(c), [](ref_type v){ return &v; });
}


/*----------------------------------------------------------------------------
                                    filter
----------------------------------------------------------------------------*/

template <typename C>
class filter_seq {
private:
    typedef std::remove_reference_t<C> C_;

public:
    typedef decltype(std::declval<C>().begin()) item_iterator;
    typedef typename std::iterator_traits<item_iterator>::value_type& item_reference;
    typedef typename std::function<bool(item_reference)> predicate;

    class iterator {
    private:
	item_iterator it_;
	const item_iterator itend_;
	predicate pred_;
	bool terminator_;

	void find() {
	    for (; it_ != itend_; ++it_) {
		if (pred_(*it_))
		    return;
	    }
	    terminator_ = true;
	}

    public:
	typedef ptrdiff_t difference_type;
	typedef typename std::iterator_traits<item_iterator>::value_type value_type;
	typedef typename std::iterator_traits<item_iterator>::pointer pointer;
	typedef typename std::iterator_traits<item_iterator>::reference reference;
	typedef std::input_iterator_tag iterator_category;

	iterator(C_& c, predicate pred):
	    it_(c.begin()), itend_(c.end()), pred_(pred), terminator_(false) {
	    find();
	}

	explicit iterator(C_& c):
	    it_(c.end()), itend_(c.end()), pred_(), terminator_(true) {
	}

	bool operator==(const iterator& rhs) const {
	    return (it_ == rhs.it_ || (terminator_ && rhs.terminator_));
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (terminator_)
		throw std::runtime_error("end iterator refuse operator++");
	    if (it_ != itend_) {
		++it_;
		find();
	    }
	    return *this;
	}

	decltype(*it_) operator*() {
	    if (terminator_)
		throw std::out_of_range("Maybe end iterator.");
	    return *it_;
	}
    };

    typedef iterator const_iterator;

private:
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_, C_&>::type S;
    typedef typename std::conditional<std::is_rvalue_reference_v<C>, C_&&, C_&>::type A;

    S c;
    predicate pred;

public:
    filter_seq(A c, predicate pred):
	c(std::forward<C>(c)), pred(pred) {
    }

    filter_seq(filter_seq<C>&& h): c(std::forward<C>(h.c)), pred(h.pred) {
    }

    filter_seq(filter_seq<C>& h): c(h.c), pred(h.pred) {
    }

    filter_seq(const filter_seq<C>& h): c(h.c), pred(h.pred) {
    }

    iterator begin() noexcept {
	return iterator(c, pred);
    }

    iterator end() noexcept {
	return iterator(c);
    }
};

template <typename C>
filter_seq<C&&> filter(C&& c, typename filter_seq<C&&>::predicate pred)
{
    return filter_seq<C&&>(std::forward<C>(c), pred);
}


/*----------------------------------------------------------------------------
                          C array - T (&)[N], T* & N
----------------------------------------------------------------------------*/

template <typename T>
class c_array {
public:
    using iterator = c7::c_array_iterator<T>;

    template <size_t N>
    explicit c_array(T (&c)[N]): c_array(c, N) {}
    c_array(T *p, size_t n): top_(p), n_(n) {}

    c_array(const c_array& o): top_(o.top_), n_(o.n_) {}
    c_array(c_array&& o): top_(o.top_), n_(o.n_) {
	o.top_ = nullptr;
	o.n_ = 0;
    }

    iterator begin() {
	return iterator(top_, 0);
    }

    iterator end() {
	return iterator(top_, n_);
    }

private:
    T *top_;
    size_t n_;
};


/*----------------------------------------------------------------------------
                      null terminated array to sequence
----------------------------------------------------------------------------*/

template <typename PP, typename Conv>
class c_parray_seq {
public:
    class iterator {
    private:
	PP array_;
	Conv conv_;
	ptrdiff_t index_;

    public:
	typedef ptrdiff_t difference_type;
	typedef std::remove_reference_t<decltype(conv_(array_[0]))> value_type;
	typedef std::remove_reference_t<value_type>* pointer;
	typedef std::remove_reference_t<value_type>& reference;
	typedef std::input_iterator_tag iterator_category;

	iterator(PP array, Conv conv, bool is_terminator):
	    array_(array), conv_(conv), index_(is_terminator ? -1 : 0) {
	}

	bool operator==(const iterator& rhs) const {
	    return (index_ == rhs.index_);
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (index_ == -1)
		throw std::runtime_error("end iterator refuse operator++");
	    if (array_[++index_] == nullptr) {
		index_ = -1;
	    }
	    return *this;
	}

	value_type operator*() {
	    if (index_ == -1)
		throw std::runtime_error("end iterator derefernce*");
	    return conv_(array_[index_]);
	}
    };

    typedef iterator const_iterator;

private:
    PP array_;
    Conv conv_;

public:
    c_parray_seq(PP array, Conv conv): array_(array), conv_(conv) {}

    c_parray_seq(c_parray_seq<PP, Conv>& o): array_(o.array_), conv_(o.conv_) {}

    c_parray_seq(c_parray_seq<PP, Conv>&& o) = delete;

    iterator begin() noexcept {
	return iterator(array_, conv_, false);
    }

    iterator end() noexcept {
	return iterator(array_, conv_, true);
    }
};

template <typename PP>
struct c_parray_traits {
    typedef std::remove_reference_t<decltype(*std::declval<PP>())> value_type;
};

template <typename PP>
auto c_parray(PP array)
{
    auto conv = [](typename c_parray_traits<PP>::value_type& p) { return p; };
    return c_parray_seq<PP, decltype(conv)>(array, conv);
}

template <typename PP, typename Conv>
c_parray_seq<PP, Conv> c_parray(PP array, Conv conv)
{
    return c_parray_seq<PP, Conv>(array, conv);
}

template <typename CharPP>
auto charp_array(CharPP array)
{
    auto conv = [](typename c_parray_traits<CharPP>::value_type& p) { return std::string(p); };
    return c_parray_seq<CharPP, decltype(conv)>(array, conv);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::seq


#endif // c7seq.hpp
