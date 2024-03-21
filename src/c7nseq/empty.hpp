/*
 * c7nseq/empty.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_EMPTY_HPP_LOADED__
#define C7_NSEQ_EMPTY_HPP_LOADED__


#include <c7typefunc.hpp>


namespace c7::nseq {


template <typename T = char>
class empty_seq {
    class iter_end {};
    class iter {
    public:
	// for STL iterator
	using iterator_category	= std::input_iterator_tag;
	using difference_type	= ptrdiff_t;
	using value_type	=  std::remove_reference_t<T>;
	using pointer		= value_type*;
	using reference		= value_type&;

	iter() = default;
	iter(const iter&) = default;
	iter(iter&&) = default;
	iter& operator=(const iter&) = default;
	iter& operator=(iter&&) = default;

	bool operator==(const iter& o) const {
	    return true;
	}

	bool operator!=(const iter& o) const {
	    return false;
	}

	bool operator==(const iter_end& o) const {
	    return true;
	}

	bool operator!=(const iter_end& o) const {
	    return false;
	}

	auto& operator++() {
	    return *this;
	}

	decltype(auto) operator*() {
	    return T{};
	}

	decltype(auto) operator*() const {
	    return T{};
	}
    };

public:
    auto size() const {
	return 0;
    }

    auto empty() const {
	return true;
    }

    auto begin() {
	return iter();
    }

    auto end() {
	return iter_end();
    }

    auto begin() const {
	return iter();
    }

    auto end() const {
	return iter_end();
    }

    auto rbegin() {
	return iter();
    }

    auto rend() {
	return iter_end();
    }

    auto rbegin() const {
	return iter();
    }

    auto rend() const {
	return iter_end();
    }
};


} // namespace c7::nseq


#endif // c7nseq/empty.hpp
