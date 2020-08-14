/*
 * c7result.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_RESULT_HPP_LOADED__
#define __C7_RESULT_HPP_LOADED__
#include <c7common.hpp>


#include <c7format.hpp>
#include <stdexcept>
#include <vector>


namespace c7 {


// result category tag and traits

struct result_move_tag {};
struct result_copy_tag {};
struct result_void_tag {};

template <typename R>
struct result_category {
    typedef typename
    std::conditional_t<std::is_void_v<R>,
		       result_void_tag,
		       std::conditional_t<std::is_scalar_v<std::remove_reference_t<R>>,
					  result_copy_tag,
					  result_move_tag>> type;
};

template <typename R>
struct result_traits {
    static R init() { return R(); }	// default constructor
};


// base class

class result_base {
public:
    struct errinfo {
	const char *file;
	int line;
	int what;
	std::string msg;

	template <typename... Args>
	errinfo(const char *f, int ln, int w, const std::string msg, const Args&... args):
	    file(f), line(ln), what(w), msg(c7::format(msg, args...)) {
	}

	errinfo(const char *f, int ln, int w):
	    file(f), line(ln), what(w), msg() {
	}
    };

protected:
    bool is_success_;
    std::vector<errinfo> infos_;

public:
    result_base(const result_base&) = delete;
    result_base& operator=(const result_base&) = delete;

    result_base(): is_success_(false) {}

    explicit result_base(bool b): is_success_(b) {}

    result_base(result_base&& o):
	is_success_(o.is_success_), infos_(std::move(o.infos_)) {
    }

    result_base& operator=(result_base&& o) {
	if (this != &o) {
	    is_success_ = o.is_success_;
	    infos_ = std::move(o.infos_);
	}
	return *this;
    }

    template <typename... Args>
    result_base(const char *file, int line, const std::string& msg, const Args&... args):
	is_success_(false) {
	infos_.push_back(errinfo(file, line, 0, msg, args...));
    }

    template <typename... Args>
    result_base(const char *file, int line, int what, const std::string& msg, const Args&... args):
	is_success_(false) {
	infos_.push_back(errinfo(file, line, what, msg, args...));
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, const std::string& msg, const Args&... args):
	is_success_(false), infos_(std::move(base.infos_)) {
	infos_.push_back(errinfo(file, line, 0, msg, args...));
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, int what, const std::string& msg, const Args&... args):
	is_success_(false), infos_(std::move(base.infos_)) {
	infos_.push_back(errinfo(file, line, what, msg, args...));
    }

    result_base(const char *file, int line, result_base&& base, int what):
	is_success_(false), infos_(std::move(base.infos_)) {
	infos_.push_back(errinfo(file, line, what));
    }

    result_base(const char *file, int line, result_base&& base):
	is_success_(false), infos_(std::move(base.infos_)) {
	infos_.push_back(errinfo(file, line, 0));
    }

    result_base(const char *file, int line, int what):
	is_success_(false) {
	infos_.push_back(errinfo(file, line, what));
    }

    template <typename... Args>
    result_base&& set_error(const char *file, int line, const std::string& msg, const Args&... args) {
	is_success_ = false;
	infos_.push_back(errinfo(file, line, 0, msg, args...));
	return std::move(*this);
    }

    template <typename... Args>
    result_base&& set_error(const char *file, int line, int what, const std::string& msg, const Args&... args) {
	is_success_ = false;
	infos_.push_back(errinfo(file, line, what, msg, args...));
	return std::move(*this);
    }

    template <typename... Args>
    result_base&& set_error(const char *file, int line, result_base&& base, int what, const std::string& msg, const Args&... args) {
	is_success_ = false;
	infos_ = std::move(base.infos_);
	infos_.push_back(errinfo(file, line, what, msg, args...));
	return std::move(*this);
    }

    result_base&& set_error(const char *file, int line, result_base&& base, int what) {
	is_success_ = false;
	infos_ = std::move(base.infos_);
	infos_.push_back(errinfo(file, line, what));
	return std::move(*this);
    }

    result_base&& set_error(const char *file, int line, result_base&& base) {
	is_success_ = false;
	infos_ = std::move(base.infos_);
	infos_.push_back(errinfo(file, line, 0));
	return std::move(*this);
    }

    explicit operator bool() const {
	return is_success_;
    }

    const std::vector<errinfo>& error() const {
	return infos_;
    }

    void clear() {
	is_success_ = true;
	infos_.clear();
    }
};


// primary template

template <typename R, typename Tag = typename result_category<R>::type>
class result: public result_base {};


// sute for move operation

template <typename R>
class result<R, result_move_tag>: public result_base {
private:
    R value_;
    static R init_() {
	return result_traits<R>::init();
    }

public:
    explicit result(): result_base(true), value_(init_()) {}

    explicit result(bool b): result_base(b), value_(init_()) {}

    explicit result(const R& r): result_base(true), value_(r) {}

    result(R&& r): result_base(true), value_(std::move(r)) {}

    result(result<R, result_move_tag>&& o):
	result_base(std::move(o)), value_(std::move(o.value_)) {
    }

    result(result_base&& o): result_base(std::move(o)), value_(init_()) {
	if (is_success_) {
	    result_base::set_error(__FILE__, __LINE__, "value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result<R, result_move_tag>&& set_error(const Args&... args) {
	result_base::set_error(args...);
	return std::move(*this);
    }

    result<R, result_move_tag>&& operator=(result<R, result_move_tag>&& o) {
	if (this != &o) {
	    is_success_ = o.is_success_;
	    if (is_success_) {
		value_ = std::move(o.value_);
	    } else {
		infos_ = std::move(o.infos_);
	    }
	}
	return std::move(*this);
    }

    result<R, result_move_tag>&& operator=(const R& r) {
	value_ = r;
	is_success_ = true;
	return std::move(*this);
    }

    result<R, result_move_tag>&& operator=(R&& r) {
	value_ = std::move(r);
	is_success_ = true;
	return std::move(*this);
    }

    R& value() {
	if (!is_success_) {
	    throw std::runtime_error("result has no value");
	}
	return value_;
    }

    R& value(const R& alt_value) {
	if (!is_success_) {
	    value_ = alt_value;
	}
	return value_;
    }

    R& value(R&& alt_value) {
	if (!is_success_) {
	    value_ = std::move(alt_value);
	}
	return value_;
    }
};


// sute for copy operation

template <typename R>
class result<R, result_copy_tag>: public result_base {
private:
    R value_;

public:
    explicit result(): result_base(true), value_() {}

    explicit result(R r): result_base(true), value_(r) {}

    result(result<R, result_copy_tag>&& o): result_base(std::move(o)), value_(o.value_) {}

    result(result_base&& o): result_base(std::move(o)), value_() {
	if (is_success_) {
	    result_base::set_error(__FILE__, __LINE__, "value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result<R, result_copy_tag>&& set_error(const Args&... args) {
	result_base::set_error(args...);
	return std::move(*this);
    }

    result<R, result_copy_tag>&& operator=(result<R, result_copy_tag>&& o) {
	if (this != &o) {
	    is_success_ = o.is_success_;
	    if (is_success_) {
		value_ = o.value_;
	    } else {
		infos_ = std::move(o.infos_);
	    }
	}
	return std::move(*this);
    }

    result<R, result_copy_tag>&& operator=(R r) {
	value_ = r;
	is_success_ = true;
	return std::move(*this);
    }

    R value() {
	if (!is_success_) {
	    throw std::runtime_error("result has no value");
	}
	return value_;
    }

    R value(R alt_value) {
	return is_success_ ? value_ : alt_value;
    }
};


// void (no data)

template <>
class result<void, result_void_tag>: public result_base {
public:
    result(): result_base(true) {}

    explicit result(bool b): result_base(b) {}

    result(result<void, result_void_tag>&& o): result_base(std::move(o)) {}

    result(result_base&& o): result_base(std::move(o)) {
	if (is_success_) {
	    result_base::set_error(__FILE__, __LINE__, "value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result<void, result_void_tag>&& set_error(const Args&... args) {
	result_base::set_error(args...);
	return std::move(*this);
    }

    result<void, result_void_tag>&& operator=(result<void, result_void_tag>&& o) {
	if (this != &o) {
	    is_success_ = o.is_success_;
	    infos_ = std::move(o.infos_);
	}
	return std::move(*this);
    }
};


template <>
struct format_traits<result_base> {
    static c7::delegate<bool, std::ostream&, int> translate_what;
    static void convert(std::ostream& out, const std::string& format, const result_base& arg);
};


template <typename R, typename Tag>
struct format_traits<result<R, Tag>> {
    static void convert(std::ostream& out, const std::string& format, const result<R, Tag>& arg) {
	format_traits<result_base>::convert(out, format, arg);
    }
};


} // namespace c7


template <typename R>
inline auto c7result_ok(R&& r)
{
    return c7::result<std::remove_reference_t<R>>(std::forward<R>(r));
}

inline auto c7result_ok()
{
    return c7::result<void>();
}

#define c7result_err(...)		c7::result_base(__FILE__, __LINE__, __VA_ARGS__)
#define c7result_seterr(r, ...)		std::move(((r).set_error(__FILE__, __LINE__, __VA_ARGS__)))


#endif // c7result.hpp
