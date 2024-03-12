/*
 * c7event/iovec_proxy.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/iovec_proxy.hpp>


namespace c7::event {


c7::result<>
iovec_proxy::size_error(size_t elm_size, size_t expect_min, size_t expect_max) const
{
    std::string s;
    if (expect_min > 0 && expect_min == expect_max) {
	s = c7::format("size mismatch: iov_len:%{}, elm_size:%{}, expected #:%{}",
		       iov_.iov_len, elm_size);
    } else if (expect_max == -1UL) {
	s = c7::format("size mismatch: iov_len:%{}, elm_size:%{}, expected min#:%{}",
		       iov_.iov_len, elm_size, expect_min);
    } else if (expect_min == 0){
	s = c7::format("size mismatch: iov_len:%{}, elm_size:%{}, expected max#:%{}",
		       iov_.iov_len, elm_size, expect_max);
    } else {
	s = c7::format("size mismatch: iov_len:%{}, elm_size:%{}, expected min#:%{}, expected max#:%{}",
		       iov_.iov_len, elm_size, expect_min, expect_max);
    }
    return c7result_err(EINVAL, s);
}


} // namespace c7::event
