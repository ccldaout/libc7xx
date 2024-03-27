/*
 * c7iters.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_ITERS_HPP_LOADED_
#define C7_ITERS_HPP_LOADED_
#include <c7common.hpp>


namespace c7 {


/*----------------------------------------------------------------------------
                               iterator wrapper
----------------------------------------------------------------------------*/

template <typename Iter, typename Conv>
class convert_iter: public Iter {
private:
    Conv conv_;

public:
    using base_type		= Iter;
    using difference_type	= typename base_type::difference_type;
    using value_type		= decltype(conv_(std::declval<typename base_type::reference>()));
    using porinter		= value_type*;
    using reference		= value_type&;
    using iterator_category	= typename base_type::iterator_category;

    convert_iter(): base_type() {}
    convert_iter(const base_type& it, Conv conv): base_type(it), conv_(conv) {}
    convert_iter(const convert_iter& o): base_type(o), conv_(o.conv_) {}
    convert_iter& operator=(const convert_iter& o) {
	base_type::operator=(o);
	conv_ = o.conv_;
	return *this;
    }
    value_type operator*() {
	return conv_(base_type::operator*());
    }
    const value_type operator*() const {
	using self_type = convert_iter<Iter, Conv>;
	return const_cast<self_type*>(this)->operator*();
    }

    template <typename It2, typename Cv2>
    bool operator==(const convert_iter<It2, Cv2>& o) const {
	return (static_cast<const Iter>(*this) ==
		static_cast<const It2>(o));
    }

    template <typename It2, typename Cv2>
    bool operator!=(const convert_iter<It2, Cv2>& o) const {
	return !(*this == o);
    }

};

template <typename Iter, typename Conv>
static inline convert_iter<Iter, Conv>
make_convert_iter(Iter it, Conv conv)
{
    return convert_iter<Iter, Conv>(it, conv);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7iters.hpp
