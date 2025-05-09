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
#ifndef C7_RESULT_HPP_LOADED_
#define C7_RESULT_HPP_LOADED_
#include <c7common.hpp>


#include <c7format.hpp>
#include <vector>


namespace c7 {


// result category tag and traits

struct result_move_tag {};
struct result_copy_tag {};
struct result_void_tag {};

template <typename R>
using result_category_t =
    c7::typefunc::ifelse_t<
    std::is_void<R>,				result_void_tag,
    std::is_scalar<std::remove_reference_t<R>>,	result_copy_tag,
    std::true_type,				result_move_tag
    >;

template <typename R>
struct result_traits {
    static R init() { return R(); }	// default constructor
};


class result_err;


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

    result_err&& as_error() {
	return reinterpret_cast<result_err&&>(*this);
    }

    const result_err& as_error() const {
	return reinterpret_cast<const result_err&>(*this);
    }

    void copy(result_base& target) const {
	target.copy_from(*this);
    }

    void copy_from(const result_base&);

    void merge_iferror(result_base&& source);

    result_base& operator<<(result_base&& source) {
	merge_iferror(std::move(source));
	return *this;
    }

    explicit operator bool() const {
	return !errors_;
    }

    const std::vector<errinfo>& error() const {
	return errors_ ? *errors_ : no_error_;
    }

    bool has_what(int what) const {
	if (errors_) {
	    for (const auto& e: *errors_) {
		if (e.what == what) {
		    return true;
		}
	    }
	}
	return false;
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

    void raise_exception() const;

private:
    static const std::vector<errinfo> no_error_;

    // Obsolete functions exists for link compatibility
    static void type_mismatch();
    static void has_no_value();
};


class result_err: public result_base {
public:
    using result_base::result_base;
    void raise() const {
	raise_exception();
    }
};


// exception
class result_exception: public std::exception {
public:
    explicit result_exception(result_err&& err): err_(std::move(err)) {}

    result_err&& as_result() {
	return std::move(err_);
    }

private:
    result_err err_;
};


// primary template

template <typename R = void, typename Tag = result_category_t<R>>
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

    result(result_err&& o): result_base(std::move(o)), value_(init_()) {
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
	    raise_exception();
	}
	return value_;
    }

    const R& value() const {
	if (errors_) {
	    raise_exception();
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

    void check() const {
	if (errors_) {
	    raise_exception();
	}
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

    result(result_err&& o): result_base(std::move(o)), value_() {
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
	    raise_exception();
	}
	return value_;
    }

    R value(R alt_value) const {
	return errors_ ? alt_value : value_;
    }

    void check() const {
	if (errors_) {
	    raise_exception();
	}
    }
};


// void (no data)

template <>
class result<void, result_void_tag>: public result_base {
public:
    result(): result_base() {}

    result(result&& o): result_base(std::move(o)) {}

    result(result_base&& o): result_base(std::move(o)) {}

    void value() const {
	if (errors_) {
	    raise_exception();
	}
    }

    void check() const {
	if (errors_) {
	    raise_exception();
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


template <typename L, typename RFunc>
auto operator>>(c7::result<L>&& res, RFunc func)
    -> decltype(func(std::move(res.value())))
{
    if (res) {
	return func(std::move(res.value()));
    } else {
	return res.as_error();
    }
}


template <typename RFunc>
auto operator>>(c7::result<void>&& res, RFunc func)
    -> decltype(func())
{
    if (res) {
	return func();
    } else {
	return res.as_error();
    }
}


template <>
struct format_traits<result_base> {
    static c7::delegate<bool, std::ostream&, int> translate_what;
    static void convert(std::ostream& out, const std::string& format, const result_base& arg);
};


template <>
struct format_traits<result_err> {
    static c7::delegate<bool, std::ostream&, int> translate_what;
    static void convert(std::ostream& out, const std::string& format, const result_err& arg) {
	format_traits<result_base>::convert(out, format, arg);
    }
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

#define c7result_err(...)		c7::result_err(__FILE__, __LINE__, __VA_ARGS__)
#define c7result_seterr(r, ...)		std::move(((r).set_error(__FILE__, __LINE__, __VA_ARGS__)))


#endif // c7result.hpp
