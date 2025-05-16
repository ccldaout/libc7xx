/*
 * c7json/proxy.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_JSON_PROXY_HPP_LOADED_
#define C7_JSON_PROXY_HPP_LOADED_
#include <c7common.hpp>


#include <unordered_map>
#include <vector>
#include <c7json/lexer.hpp>


#define c7json_init(...)					\
    std::pair<std::unordered_map<std::string, c7::json::proxy_object::proxy_ops>&,	\
	      std::vector<std::string>&>			\
    proxy_attribute_() const override {					\
	using self_type [[maybe_unused]] = c7::typefunc::remove_cref_t<decltype(*this)>; \
	static std::vector<std::string> def_order;			\
	static std::unordered_map<std::string, c7::json::proxy_object::proxy_ops> ops_map = { \
	    __VA_ARGS__							\
	};								\
	return {ops_map, def_order};					\
    }

#define c7json_member(n)	member_attribute_(def_order, #n, &self_type::n)


namespace c7::json {


/*----------------------------------------------------------------------------
                                 proxy_basic
----------------------------------------------------------------------------*/

struct dump_context {
    int indent = 0;
    int cur_indent = 0;
    int time_prec = 6;		// 0:sec, 3:milli sec, 6:micro sec
};


template <typename T>
class proxy_basic {
public:
    proxy_basic(): val_(), has_(false) {}

    proxy_basic(const T& v): val_(v), has_(true) {}
    proxy_basic(T&& v): val_(std::move(v)), has_(true) {}

    proxy_basic(const proxy_basic&) = default;
    proxy_basic(proxy_basic&&) = default;
    proxy_basic& operator=(const proxy_basic&) = default;
    proxy_basic& operator=(proxy_basic&&) = default;

    c7::result<> load(lexer&, token&);

    c7::result<> dump(std::ostream&, dump_context&);

    void clear() {
	T dummy{};
	std::swap(val_, dummy);
	has_ = false;
    }

    bool check() const {
	return has_;
    }

    T& operator()() {
	return val_;
    }

    T& operator()() const {
	return val_;
    }

    operator T&() {
	return val_;
    }

    operator const T&() const {
	return val_;
    }

    template <typename U = T>
    proxy_basic& operator=(U&& v) {
	val_ = std::forward<U>(v);
	has_ = true;
	return *this;
    }

    decltype(auto) print_as() const {
	if constexpr(std::is_fundamental_v<T>) {
	    return val_;
	} else {
	    return (val_);	// type of (val_) is reference
	}
    }

private:
    T val_;
    bool has_;
};


struct time_us_tag {};
using time_us = c7::simple_wrap<c7::usec_t, time_us_tag>;

using binary_t = std::vector<uint8_t>;

template <> c7::result<> proxy_basic<int64_t    >::load(lexer&, token&);
template <> c7::result<> proxy_basic<int64_t    >::dump(std::ostream&, dump_context&);
template <> c7::result<> proxy_basic<double     >::load(lexer&, token&);
template <> c7::result<> proxy_basic<double     >::dump(std::ostream&, dump_context&);
template <> c7::result<> proxy_basic<bool       >::load(lexer&, token&);
template <> c7::result<> proxy_basic<bool       >::dump(std::ostream&, dump_context&);
template <> c7::result<> proxy_basic<std::string>::load(lexer&, token&);
template <> c7::result<> proxy_basic<std::string>::dump(std::ostream&, dump_context&);
template <> c7::result<> proxy_basic<time_us    >::load(lexer&, token&);
template <> c7::result<> proxy_basic<time_us    >::dump(std::ostream&, dump_context&);
template <> c7::result<> proxy_basic<binary_t   >::load(lexer&, token&);
template <> c7::result<> proxy_basic<binary_t   >::dump(std::ostream&, dump_context&);


/*----------------------------------------------------------------------------
                                 proxy_array
----------------------------------------------------------------------------*/

class proxy_array_base {
public:
    virtual ~proxy_array_base() = default;

    c7::result<> load(lexer&, token&);

protected:
    c7::result<> dump_all(std::ostream&, dump_context&, size_t);
    virtual c7::result<> load_element(lexer&, token&) = 0;
    virtual c7::result<> dump_element(std::ostream&, dump_context&, size_t) = 0;
};


template <typename Proxy>
class proxy_array: public proxy_array_base {
public:
    proxy_array() = default;

    proxy_array(std::initializer_list<Proxy> lst): vec_(lst) {}

    proxy_array(const proxy_array&) = default;
    proxy_array(proxy_array&&) = default;
    proxy_array& operator=(const proxy_array&) = default;
    proxy_array& operator=(proxy_array&&) = default;

    // inherit load()

    c7::result<> dump(std::ostream& o, dump_context& dc) {
	return dump_all(o, dc, vec_.size());
    }

    void clear() {
	vec_.clear();
    }

    bool check() const {
	for (auto& v: vec_) {
	    if (!v.check()) {
		return false;
	    }
	}
	return true;
    }

    decltype(auto) operator[](ssize_t idx) {
	return vec_[idx];
    }

    decltype(auto) operator[](ssize_t idx) const {
	return vec_[idx];
    }

    template <typename U = Proxy>
    proxy_array& push_back(U&& v) {
	vec_.push_back(std::forward<U>(v));
	return *this;
    }

    template <typename... Args>
    proxy_array& emplace_back(Args... args) {
	vec_.emplace_back(std::forward<Args>(args)...);
	return *this;
    }

    size_t size() const {
	return vec_.size();
    }

    Proxy& extend() {
	return vec_.emplace_back();
    }

    auto begin() {
	return vec_.begin();
    }

    auto begin() const {
	return vec_.begin();
    }

    auto end() {
	return vec_.end();
    }

    auto end() const {
	return vec_.end();
    }

    std::vector<Proxy>& operator()() {
	return vec_;
    }

    std::vector<Proxy>& operator()() const {
	return vec_;
    }

private:
    std::vector<Proxy> vec_;

    c7::result<> load_element(lexer& lxr, token& t) override {
	Proxy p;
	if (auto res = p.load(lxr, t); !res) {
	    return res;
	}
	vec_.push_back(std::move(p));
	return c7result_ok();
    }

    c7::result<> dump_element(std::ostream& o, dump_context& dc, size_t i) override {
	return vec_[i].dump(o, dc);
    }
};


/*----------------------------------------------------------------------------
                               proxy_unconcern
----------------------------------------------------------------------------*/

class proxy_unconcern {
public:
    proxy_unconcern() = default;

    proxy_unconcern(const proxy_unconcern&) = default;
    proxy_unconcern(proxy_unconcern&&) = default;
    proxy_unconcern& operator=(const proxy_unconcern&) = default;
    proxy_unconcern& operator=(proxy_unconcern&&) = default;

    c7::result<> load(lexer&, token&);
    c7::result<> dump(std::ostream&, dump_context&);
    void clear();
    bool check() const;

private:
    struct raw_token {
	raw_token(token_code c, const std::string s): code(c), raw_str(s) {}
	token_code code;
	std::string raw_str;
    };

    std::vector<raw_token> tkns_;

    c7::result<> load_value(lexer&, token&);
    c7::result<> load_array(lexer&, token&);
    c7::result<> load_object(lexer&, token&);
    void store(token&);
};


/*----------------------------------------------------------------------------
                                 proxy_object
----------------------------------------------------------------------------*/

class proxy_object {
public:
    struct proxy_ops {
	std::function<c7::result<>(proxy_object *, lexer&, token&)> load;
	std::function<c7::result<>(proxy_object *, std::ostream&, dump_context&)> dump;
	std::function<void(proxy_object *)> clear;
	std::function<bool(const proxy_object *)> check;
    };

    proxy_object() = default;

    proxy_object(const proxy_object&) = default;
    proxy_object(proxy_object&&) = default;
    proxy_object& operator=(const proxy_object&) = default;
    proxy_object& operator=(proxy_object&&) = default;

    c7::result<> load(lexer&, token&);
    c7::result<> dump(std::ostream&, dump_context&);
    void clear();
    bool check() const;

    virtual std::pair<std::unordered_map<std::string, proxy_ops>&,
		      std::vector<std::string>&>
    proxy_attribute_() const = 0;

protected:
    template <typename Derived, typename Proxy>
    std::pair<std::string, proxy_ops>
    member_attribute_(std::vector<std::string>& order,
		     const std::string& name,
		     Proxy Derived::*member) const {
	order.push_back(name);
	return std::pair<std::string, proxy_ops>{
	    name,
	    proxy_ops{
		[member](proxy_object *self, auto& lxr, auto &t) {
		    return (static_cast<Derived*>(self)->*member).load(lxr, t);
		},
		[member](proxy_object *self, auto& out, auto &dc) {
		    return (static_cast<Derived*>(self)->*member).dump(out, dc);
		},
		[member](proxy_object *self) {
		    (static_cast<Derived*>(self)->*member).clear();
		},
		[member](const proxy_object *self) {
		    return (static_cast<const Derived*>(self)->*member).check();
		}
	    }
	};
    }

private:
    std::unique_ptr<std::unordered_map<std::string, proxy_unconcern>> unconcerns_;
    std::vector<std::string> src_order_;
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::json


#endif // c7json/proxy.hpp
