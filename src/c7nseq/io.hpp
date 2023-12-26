/*
 * c7nseq/io.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_IO_HPP_LOADED__
#define C7_NSEQ_IO_HPP_LOADED__


#include <ios>
#include <iterator>
#include <c7file.hpp>
#include <c7fd.hpp>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/c_array.hpp>


namespace c7::nseq {


/*----------------------------------------------------------------------------
                                file - mmap_r
----------------------------------------------------------------------------*/

template <typename T>
auto mmap_r(const std::string& path)
{
    size_t z = 0;
    auto p = std::move(c7::file::mmap_r<T>(path, z).value());
    return c7::nseq::c_array(std::move(p), z);
}


template <typename T>
auto mmap_r(int dirfd, const std::string& path)
{
    size_t z = 0;
    auto fd = std::move(c7::open(dirfd, path).value());
    auto p = std::move(c7::file::mmap_r<T>(fd, z).value());
    return c7::nseq::c_array(std::move(p), z);
}


/*----------------------------------------------------------------------------
                                   istream
----------------------------------------------------------------------------*/

template <typename T, typename IS>
class istream_seq {
public:
    istream_seq(IS&& is, bool noskipws): is_(std::move(is)) {
	if (noskipws) {
	    is_ >> std::noskipws;
	}
    }

    istream_seq(istream_seq&&) = default;
    istream_seq& operator=(istream_seq&&) = default;

    auto begin() {
	return std::istream_iterator<T>(is_);
    }

    auto end() {
	return std::istream_iterator<T>();
    }

private:
    IS is_;
};

template <typename T, typename IS>
auto istream(IS&& is, bool noskipws=true)
{
    return istream_seq<T, IS>(std::forward<IS>(is), noskipws);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {

template <typename T, typename IS>
struct format_ident<c7::nseq::istream_seq<T, IS>> {
    static constexpr const char *name = "istream";
};

} // namespace c7::format_helper
#endif


#endif // c7nseq/io.hpp
