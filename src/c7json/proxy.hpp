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
#include <c7hash.hpp>
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

#define c7json_member2(jn, pn)	member_attribute_(def_order, #jn, &self_type::pn)


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

    template <typename U>
    proxy_basic(U&& v): val_(std::forward<U>(v)), has_(true) {}

    proxy_basic(const proxy_basic&) = default;
    proxy_basic(proxy_basic&&) = default;
    proxy_basic& operator=(const proxy_basic&) = default;
    proxy_basic& operator=(proxy_basic&&) = default;

    c7::result<> load(lexer&, token&);

    c7::result<> dump(std::ostream&, dump_context&) const;

    c7::result<> from_str(token&);

    void clear() {
	T dummy{};
	std::swap(val_, dummy);
	has_ = false;
    }

    bool check() const {
	return has_;
    }

    T& operator()() {
	return (val_);
    }

    const T& operator()() const {
	return (val_);
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


template <typename T>
bool operator==(const proxy_basic<T>& a, const proxy_basic<T>& b)
{
    return a() == b();
}


struct time_us_tag {};
using time_us = c7::simple_wrap<c7::usec_t, time_us_tag>;

using binary_t = std::vector<uint8_t>;

template <> c7::result<> proxy_basic<int64_t    >::load(lexer&, token&);
template <> c7::result<> proxy_basic<int64_t    >::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<int64_t    >::from_str(token&);
template <> c7::result<> proxy_basic<double     >::load(lexer&, token&);
template <> c7::result<> proxy_basic<double     >::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<double     >::from_str(token&);
template <> c7::result<> proxy_basic<bool       >::load(lexer&, token&);
template <> c7::result<> proxy_basic<bool       >::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<bool       >::from_str(token&);
template <> c7::result<> proxy_basic<std::string>::load(lexer&, token&);
template <> c7::result<> proxy_basic<std::string>::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<std::string>::from_str(token&);
template <> c7::result<> proxy_basic<time_us    >::load(lexer&, token&);
template <> c7::result<> proxy_basic<time_us    >::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<time_us    >::from_str(token&);
template <> c7::result<> proxy_basic<binary_t   >::load(lexer&, token&);
template <> c7::result<> proxy_basic<binary_t   >::dump(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic<binary_t   >::from_str(token&);


/*----------------------------------------------------------------------------
                                 proxy_array
----------------------------------------------------------------------------*/

class proxy_array_base {
public:
    virtual ~proxy_array_base() = default;

    c7::result<> load(lexer&, token&);

protected:
    c7::result<> dump_all(std::ostream&, dump_context&, size_t) const;
    virtual c7::result<> load_element(lexer&, token&) = 0;
    virtual c7::result<> dump_element(std::ostream&, dump_context&, size_t) const = 0;
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

    c7::result<> dump(std::ostream& o, dump_context& dc) const {
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
    proxy_array& emplace_back(Args&&... args) {
	vec_.emplace_back(std::forward<Args>(args)...);
	return *this;
    }

    size_t size() const {
	return vec_.size();
    }

    bool empty() const {
	return vec_.empty();
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

    decltype(auto) operator()() {
	return (vec_);		// (...) is required to return reference !!
    }

    decltype(auto) operator()() const {
	return (vec_);		// (...) is required to return reference !!
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

    c7::result<> dump_element(std::ostream& o, dump_context& dc, size_t i) const override {
	return vec_[i].dump(o, dc);
    }
};


/*----------------------------------------------------------------------------
                                  proxy_dict
----------------------------------------------------------------------------*/

class proxy_dict_base {
public:
    virtual ~proxy_dict_base() = default;

    c7::result<> load(lexer&, token&);

protected:
    c7::result<> dump_all(std::ostream&, dump_context&) const;
    virtual c7::result<> load_element(lexer&, token&, token& key) = 0;
    virtual c7::result<> dump_element(std::ostream&, dump_context&) const = 0;
    virtual bool dump_start() const = 0;
    virtual bool dump_next() const = 0;
};


template <typename KeyProxy, typename ValueProxy>
class proxy_dict: public proxy_dict_base {
public:
    proxy_dict() = default;

    proxy_dict(std::initializer_list<std::pair<KeyProxy, ValueProxy>> lst): dic_(lst) {}

    proxy_dict(const proxy_dict&) = default;
    proxy_dict(proxy_dict&&) = default;
    proxy_dict& operator=(const proxy_dict&) = default;
    proxy_dict& operator=(proxy_dict&&) = default;

    // inherit load()

    c7::result<> dump(std::ostream& o, dump_context& dc) const {
	return dump_all(o, dc);
    }

    void clear() {
	dic_.clear();
    }

    bool check() const {
	return !dic_.empty();
    }

    template <typename U = KeyProxy, typename V = ValueProxy>
    proxy_dict& insert_or_assign(U&& k, V&& v) {
	dic_.insert_or_assign(std::forward<U>(k), std::forward<V>(v));
	return *this;
    }

    auto find(const KeyProxy& k) {
	return dic_.find(k);
    }

    auto find(const KeyProxy& k) const {
	return dic_.find(k);
    }

    size_t size() const {
	return dic_.size();
    }

    bool empty() const {
	return dic_.empty();
    }

    auto begin() {
	return dic_.begin();
    }

    auto begin() const {
	return dic_.begin();
    }

    auto end() {
	return dic_.end();
    }

    auto end() const {
	return dic_.end();
    }

    decltype(auto) operator()() {
	return (dic_);		// (...) is required to return reference !!
    }

    decltype(auto) operator()() const {
	return (dic_);		// (...) is required to return reference !!
    }

private:
    std::unordered_map<KeyProxy, ValueProxy> dic_;
    mutable decltype(dic_.cbegin()) it_;

    c7::result<> load_element(lexer& lxr, token& t, token& key_str) override {
	KeyProxy k;
	if (auto res = k.from_str(key_str); !res) {
	    return res;
	}
	ValueProxy v;
	if (auto res = v.load(lxr, t); !res) {
	    return res;
	}
	dic_.insert_or_assign(std::move(k), std::move(v));
	return c7result_ok();
    }

    bool dump_start() const override {
	it_ = dic_.cbegin();
	return (it_ != dic_.cend());
    }

    bool dump_next() const override {
	++it_;
	return (it_ != dic_.cend());
    }

    c7::result<> dump_element(std::ostream& o, dump_context& dc) const override {
	auto& [k, v] = *it_;
	if constexpr (std::is_same_v<KeyProxy, proxy_basic<int>> ||
		      std::is_same_v<KeyProxy, proxy_basic<double>> ||
		      std::is_same_v<KeyProxy, proxy_basic<bool>>) {
	    o << '"';
	    if (auto res = k.dump(o, dc); !res) {
		return res;
	    }
	    o << '"';
	} else {
	    if (auto res = k.dump(o, dc); !res) {
		return res;
	    }
	}
	o << ':';
	return v.dump(o, dc);
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
    c7::result<> dump(std::ostream&, dump_context&) const;
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
	std::function<c7::result<>(const proxy_object *, std::ostream&, dump_context&)> dump;
	std::function<void(proxy_object *)> clear;
	std::function<bool(const proxy_object *)> check;
    };

    proxy_object() = default;

    proxy_object(const proxy_object&) = default;
    proxy_object(proxy_object&&) = default;
    proxy_object& operator=(const proxy_object&) = default;
    proxy_object& operator=(proxy_object&&) = default;

    c7::result<> load(lexer&, token&);
    c7::result<> dump(std::ostream&, dump_context&) const;
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
		[member](const proxy_object *self, auto& out, auto &dc) {
		    return (static_cast<const Derived*>(self)->*member).dump(out, dc);
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
    std::shared_ptr<std::unordered_map<std::string, proxy_unconcern>> unconcerns_;
    std::vector<std::string> src_order_;
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::json


namespace std {

template <typename T>
struct hash<c7::json::proxy_basic<T>>: public hash<T> {
    size_t operator()(c7::json::proxy_basic<T> k) const {
	return hash<T>()(k);
    }
};

} // namespace std


#endif // c7json/proxy.hpp
