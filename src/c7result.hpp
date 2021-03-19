/*
 * c7result.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=360068048
 */
#ifndef C7_RESULT_HPP_LOADED__
#define C7_RESULT_HPP_LOADED__
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

	errinfo(const char *f, int ln, int w, const std::string& m):
	    file(f), line(ln), what(w), msg(m) {
	}

	errinfo(const char *f, int ln, int w, std::string&& m):
	    file(f), line(ln), what(w), msg(std::move(m)) {
	}

	errinfo(const char *f, int ln, int w):
	    file(f), line(ln), what(w), msg() {
	}
    };

    result_base(const result_base&) = delete;
    result_base& operator=(const result_base&) = delete;

    result_base() {}

    result_base(result_base&& o):
	errors_(std::move(o.errors_)) {
    }

    result_base& operator=(result_base&& o) {
	if (this != &o) {
	    errors_ = std::move(o.errors_);
	}
	return *this;
    }

    template <typename... Args>
    result_base(const char *file, int line, const char *msg, const Args&... args) {
	add_errinfo(file, line, 0, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, const std::string& msg, const Args&... args) {
	add_errinfo(file, line, 0, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, int what, const char *msg, const Args&... args) {
	add_errinfo(file, line, what, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, int what, const std::string& msg, const Args&... args) {
	add_errinfo(file, line, what, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, const char *msg, const Args&... args):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, 0, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, const std::string& msg, const Args&... args):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, 0, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, int what, const char *msg, const Args&... args):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, what, msg, args...);
    }

    template <typename... Args>
    result_base(const char *file, int line, result_base&& base, int what, const std::string& msg, const Args&... args):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, what, msg, args...);
    }

    result_base(const char *file, int line, result_base&& base, int what):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, what);
    }

    result_base(const char *file, int line, result_base&& base):
	errors_(std::move(base.errors_)) {
	add_errinfo(file, line, 0);
    }

    result_base(const char *file, int line, int what) {
	add_errinfo(file, line, what);
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, const char *msg, const Args&... args) {
	errors_.reset();
	add_errinfo(file, line, 0, msg, args...);
	return *this;
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, const std::string& msg, const Args&... args) {
	errors_.reset();
	add_errinfo(file, line, 0, msg, args...);
	return *this;
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, int what, const char *msg, const Args&... args) {
	errors_.reset();
	add_errinfo(file, line, what, msg, args...);
	return *this;
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, int what, const std::string& msg, const Args&... args) {
	errors_.reset();
	add_errinfo(file, line, what, msg, args...);
	return *this;
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, result_base&& base, int what, const char *msg, const Args&... args) {
	errors_ = std::move(base.errors_);
	add_errinfo(file, line, what, msg, args...);
	return *this;
    }

    template <typename... Args>
    result_base& set_error(const char *file, int line, result_base&& base, int what, const std::string& msg, const Args&... args) {
	errors_ = std::move(base.errors_);
	add_errinfo(file, line, what, msg, args...);
	return *this;
    }

    result_base& set_error(const char *file, int line, result_base&& base, int what) {
	errors_ = std::move(base.errors_);
	add_errinfo(file, line, what);
	return *this;
    }

    result_base& set_error(const char *file, int line, result_base&& base) {
	errors_ = std::move(base.errors_);
	add_errinfo(file, line, 0);
	return *this;
    }

    void copy(result_base& target) const {
	target.copy_from(*this);
    }

    void merge_iferror(result_base&& source);

    explicit operator bool() const {
	return !errors_;
    }

    const std::vector<errinfo>& error() const {
	return errors_ ? *errors_ : no_error_;
    }

    void clear() {
	errors_.reset();
    }

protected:
    std::unique_ptr<std::vector<errinfo>> errors_;

    void add_errinfo(const char *file, int line, int what);
    void add_errinfo(const char *file, int line, int what, const char *msg);
    void add_errinfo(const char *file, int line, int what, std::string&& msg);
    void add_errinfo(const char *file, int line, int what, const std::string& msg);

    template <typename Arg1, typename... Args>
    void add_errinfo(const char *file, int line, int what, const char *msg, const Arg1& arg1, const Args&... args) {
	add_errinfo(file, line, what, c7::format(msg, arg1, args...));
    }

    template <typename Arg1, typename... Args>
    void add_errinfo(const char *file, int line, int what, const std::string& msg, const Arg1& arg1, const Args&... args) {
	add_errinfo(file, line, what, c7::format(msg, arg1, args...));
    }

    void copy_from(const result_base&);

private:
    static const std::vector<errinfo> no_error_;
};


// primary template

template <typename R = void, typename Tag = typename result_category<R>::type>
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
    explicit result(): result_base(), value_(init_()) {}

    explicit result(const R& r): result_base(), value_(r) {}

    result(R&& r): result_base(), value_(std::move(r)) {}

    result(result&& o):
	result_base(std::move(o)), value_(std::move(o.value_)) {
    }

    result(result_base&& o): result_base(std::move(o)), value_(init_()) {
	if (*this) {
	    //result_base::set_error(__FILE__, __LINE__, "value type mismatch: data maybe lost");
	    throw std::runtime_error("value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result&& set_error(const Args&... args) {
	result_base::set_error(args...);
	return std::move(*this);
    }

    result& operator=(result&& o) {
	if (this != &o) {
	    result_base::operator=(std::move(o));
	    value_ = std::move(o.value_);
	}
	return *this;
    }

    result& operator=(const R& r) {
	value_ = r;
	errors_.reset();
	return *this;
    }

    result& operator=(R&& r) {
	value_ = std::move(r);
	errors_.reset();
	return *this;
    }

    R& value() {
	if (errors_) {
	    throw std::runtime_error("result has no value");
	}
	return value_;
    }

    const R& value() const {
	if (errors_) {
	    throw std::runtime_error("result has no value");
	}
	return value_;
    }

    R& value(R&& alt_value) {
	if (errors_) {
	    return alt_value;
	}
	return value_;
    }
    const R& value(const R& alt_value) const {
	if (errors_) {
	    return alt_value;
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
    explicit result(): result_base(), value_() {}

    explicit result(R r): result_base(), value_(r) {}

    result(result&& o): result_base(std::move(o)), value_(o.value_) {}

    result(result_base&& o): result_base(std::move(o)), value_() {
	if (!errors_) {
	    throw std::runtime_error("value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result& set_error(const Args&... args) {
	result_base::set_error(args...);
	return *this;
    }

    result& operator=(result&& o) {
	if (this != &o) {
	    result_base::operator=(std::move(o));
	    value_ = o.value_;
	}
	return *this;
    }

    result& operator=(R r) {
	value_ = r;
	errors_.reset();
	return *this;
    }

    R value() const {
	if (errors_) {
	    throw std::runtime_error("result has no value");
	}
	return value_;
    }

    R value(R alt_value) const {
	return errors_ ? alt_value : value_;
    }
};


// void (no data)

template <>
class result<void, result_void_tag>: public result_base {
public:
    result(): result_base() {}

    result(result&& o): result_base(std::move(o)) {}

    result(result_base&& o): result_base(std::move(o)) {
	if (!errors_) {
	    throw std::runtime_error("value type mismatch: data maybe lost");
	}
    }

    template <typename... Args>
    result& set_error(const Args&... args) {
	result_base::set_error(args...);
	return *this;
    }

    result& operator=(result&& o) {
	if (this != &o) {
	    result_base::operator=(std::move(o));
	}
	return *this;
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
    return c7::result<>();
}

#define c7result_err(...)		c7::result_base(__FILE__, __LINE__, __VA_ARGS__)
#define c7result_seterr(r, ...)		std::move(((r).set_error(__FILE__, __LINE__, __VA_ARGS__)))


#endif // c7result.hpp
