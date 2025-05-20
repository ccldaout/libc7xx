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


#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <c7hash.hpp>
#include <c7json/lexer.hpp>


#define c7json_init(...)						\
    std::pair<std::unordered_map<std::string, c7::json::proxy_object::proxy_ops>&, \
	      std::vector<std::string>&>				\
    proxy_attribute_() const {						\
	using self_type [[maybe_unused]] = c7::typefunc::remove_cref_t<decltype(*this)>; \
	static std::vector<std::string> def_order;			\
	static std::unordered_map<std::string, c7::json::proxy_object::proxy_ops> ops_map = { \
	    __VA_ARGS__							\
	};								\
	return {ops_map, def_order};					\
    }									\
    c7::result<> load(c7::json::lexer& lxr, c7::json::token& t) {	\
	return load_impl(lxr, t, proxy_attribute_());			\
    }									\
    c7::result<> dump(std::ostream& o, c7::json::dump_context& dc) const { \
	return dump_impl(o, dc, proxy_attribute_());			\
    }									\
    void clear() {							\
	return clear_impl(proxy_attribute_());				\
    }


#define c7json_member(n)	member_attribute_(def_order, #n, &self_type::n)

#define c7json_member2(jn, pn)	member_attribute_(def_order, #jn, &self_type::pn)


namespace c7::json {


void jsonize_string(std::ostream& o, const std::string& s);


/*----------------------------------------------------------------------------
                                 proxy_basic
----------------------------------------------------------------------------*/

struct dump_context {
    int indent = 0;
    int cur_indent = 0;
    int time_prec = 6;		// 0:sec, 3:milli sec, 6:micro sec
};


template <typename T, typename Tag = void>
class proxy_basic_strict {
public:
    proxy_basic_strict(): val_() {}

    template <typename U,
	      typename = std::enable_if_t<!std::is_same_v<proxy_basic_strict<T, Tag>,
							  c7::typefunc::remove_cref_t<U>>>>
    proxy_basic_strict(U&& v): val_(std::forward<U>(v)) {}

    proxy_basic_strict(const proxy_basic_strict&) = default;
    proxy_basic_strict(proxy_basic_strict&&) = default;
    proxy_basic_strict& operator=(const proxy_basic_strict&) = default;
    proxy_basic_strict& operator=(proxy_basic_strict&&) = default;

    c7::result<> load(lexer& lxr, token& t) {
	auto p = static_cast<void *>(this);
	return static_cast<proxy_basic_strict<T>*>(p)->load_impl(lxr, t);
    }

    c7::result<> dump(std::ostream& o, dump_context& dc) const {
	auto p = static_cast<const void *>(this);
	return static_cast<const proxy_basic_strict<T>*>(p)->dump_impl(o, dc);
    }

    c7::result<> from_str(token&);

    void clear() {
	T dummy{};
	std::swap(val_, dummy);
    }

    T& operator()() {
	return (val_);
    }

    const T& operator()() const {
	return (val_);
    }

    template <typename U,
	      typename = std::enable_if_t<!std::is_same_v<proxy_basic_strict<T, Tag>,
							  c7::typefunc::remove_cref_t<U>>>>
    proxy_basic_strict& operator=(U&& v) {
	val_ = std::forward<U>(v);
	return *this;
    }

    decltype(auto) print_as() const {
	if constexpr(std::is_fundamental_v<T>) {
	    return val_;
	} else {
	    return (val_);	// type of (val_) is reference
	}
    }

    c7::result<> load_impl(lexer&, token&);
    c7::result<> dump_impl(std::ostream&, dump_context&) const;

protected:
    T val_;
};


template <typename T, typename Tag = void>
class proxy_basic: public proxy_basic_strict<T, Tag> {
public:
    using proxy_basic_strict<T, Tag>::proxy_basic_strict;

    template <typename U>
    proxy_basic(U&& v): proxy_basic_strict<T, Tag>(std::forward<U>(v)) {}

    template <typename U = T>
    proxy_basic& operator=(U&& v) {
	this->val_ = std::forward<U>(v);
	return *this;
    }

    operator T& () {
	return (this->val_);
    }

    operator const T& () const {
	return (this->val_);
    }
};


template <typename T, typename Tag>
bool operator==(const proxy_basic_strict<T, Tag>& a, const proxy_basic_strict<T, Tag>& b)
{
    return a() == b();
}

template <typename T, typename Tag>
bool operator!=(const proxy_basic_strict<T, Tag>& a, const proxy_basic_strict<T, Tag>& b)
{
    return a() != b();
}


struct time_us_tag {};
using time_us = c7::simple_wrap<c7::usec_t, time_us_tag>;

using binary_t = std::vector<uint8_t>;

template <> c7::result<> proxy_basic_strict<int64_t    >::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<int64_t    >::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<double     >::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<double     >::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<bool       >::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<bool       >::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<std::string>::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<std::string>::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<std::string>::from_str(token&);
template <> c7::result<> proxy_basic_strict<time_us    >::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<time_us    >::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<time_us    >::from_str(token&);
template <> c7::result<> proxy_basic_strict<binary_t   >::load_impl(lexer&, token&);
template <> c7::result<> proxy_basic_strict<binary_t   >::dump_impl(std::ostream&, dump_context&) const;
template <> c7::result<> proxy_basic_strict<binary_t   >::from_str(token&);


/*----------------------------------------------------------------------------
                                  proxy_pair
----------------------------------------------------------------------------*/

class proxy_pair_base {
protected:
    using load_func = std::function<c7::result<>(lexer&, token&)>;
    using dump_func = std::function<c7::result<>(std::ostream&, dump_context&, const std::string&)>;

    c7::result<> load_impl(lexer&, token&, load_func);
    c7::result<> dump_impl(std::ostream& o, dump_context& dc, dump_func) const;
};


template <typename P1, typename P2>
class proxy_pair: public proxy_pair_base {
public:
    proxy_pair() = default;

    template <typename A1, typename A2>
    proxy_pair(A1&& a1, A2&& a2): pair_(std::forward<A1>(a1), std::forward<A2>(a2)) {}

    proxy_pair(const proxy_pair&) = default;
    proxy_pair(proxy_pair&&) = default;
    proxy_pair& operator=(const proxy_pair&) = default;
    proxy_pair& operator=(proxy_pair&&) = default;

    c7::result<> load(lexer& lxr, token& t) {
	return load_impl(lxr, t,
			 [this](auto& lxr, auto& t) {
			     return load_elements(lxr, t);
			 });
    }

    c7::result<> dump(std::ostream& o, dump_context& dc) const {
	return dump_impl(o, dc,
			 [this](auto& o, auto& dc, auto& pref) {
			     return dump_elements(o, dc, pref);
			 });
    }

    void clear() {
	pair_.first.clear();
	pair_.second.clear();
    }

    decltype(auto) operator()() {
	return (pair_);		// (...) is required to return reference !!
    }

    decltype(auto) operator()() const {
	return (pair_);		// (...) is required to return reference !!
    }

private:
    std::pair<P1, P2> pair_;

    c7::result<>
    load_elements(lexer& lxr, token& t) {
	if (auto res = pair_.first.load(lxr, t); !res) {
	    return res;
	}
	if (lxr.get(t) != TKN_COMMA) {
	    return c7result_err(EINVAL, "comma (,) is required: %{}", t);
	}
	lxr.get(t);
	if (auto res = pair_.second.load(lxr, t); !res) {
	    return res;
	}
	return c7result_ok();
    }

    c7::result<>
    dump_elements(std::ostream& o, dump_context& dc, const std::string& pref) const {
	if (auto res = pair_.first.dump(o, dc); !res) {
	    return res;
	}
	o << pref;
	if (auto res = pair_.second.dump(o, dc); !res) {
	    return res;
	}
	return c7result_ok();
    }
};


template <typename P1, typename P2>
bool operator==(const proxy_pair<P1, P2>& a,
		const proxy_pair<P1, P2>& b)
{
    return a() == b();
}

template <typename P1, typename P2>
bool operator!=(const proxy_pair<P1, P2>& a,
		const proxy_pair<P1, P2>& b)
{
    return a() != b();
}


/*----------------------------------------------------------------------------
                                 proxy_tuple
----------------------------------------------------------------------------*/

class proxy_tuple_base {
public:
protected:
    using load_func = std::function<c7::result<>(lexer&, token&)>;
    using dump_func = std::function<c7::result<>(std::ostream&, dump_context&,
						 const char *, const char *)>;

    c7::result<> load_impl(lexer&, token&, load_func);
    c7::result<> dump_impl(std::ostream& o, dump_context& dc, dump_func) const;
};


template <typename... Proxies>
class proxy_tuple: public proxy_tuple_base {
public:
    proxy_tuple() = default;

    template <typename... Args>
    proxy_tuple(Args&&... args): tuple_(std::forward<Args>(args)...) {}

    proxy_tuple(const proxy_tuple&) = default;
    proxy_tuple(proxy_tuple&&) = default;
    proxy_tuple& operator=(const proxy_tuple&) = default;
    proxy_tuple& operator=(proxy_tuple&&) = default;

    c7::result<> load(lexer& lxr, token& t) {
	return load_impl(lxr, t,
			 [this](auto& lxr, auto& t) {
			     return load_elements(lxr, t);
			 });
    }

    c7::result<> dump(std::ostream& o, dump_context& dc) const {
	return dump_impl(o, dc,
			 [this](auto& o, auto& dc, auto s1, auto s2) {
			     return dump_elements(o, dc, s1, s2);
			 });
    }

    void clear() {
	clear_impl<0>();
    }

    template <size_t N>
    constexpr decltype(auto) get() {
	return std::get<N>(tuple_);
    }

    template <size_t N>
    constexpr decltype(auto) get() const {
	return std::get<N>(tuple_);
    }

    constexpr size_t size() const {
	return sizeof...(Proxies);
    }

    decltype(auto) operator()() {
	return (tuple_);		// (...) is required to return reference !!
    }

    decltype(auto) operator()() const {
	return (tuple_);		// (...) is required to return reference !!
    }

private:
    std::tuple<Proxies...> tuple_;

    template <size_t I>
    void clear_impl() {
	if constexpr (I < size()) {
	    get<I>().clear();
	    clear_impl<I+1>();
	}
    }

    c7::result<> load_elements(lexer& lxr, token& t) {
	return load_elements_impl<0>(lxr, t);
    }

    template <size_t I>
    c7::result<> load_elements_impl(lexer& lxr, token& t) {
	if constexpr (I == size()) {
	    lxr.get(t);
	    return c7result_ok();
	} else {
	    if constexpr (I != 0) {
		if (lxr.get(t) != TKN_COMMA) {
		    return c7result_err(EINVAL, "comman (,) is expected: %{}", t);
		}
		lxr.get(t);
	    }
	    if (auto res = get<I>().load(lxr, t); !res) {
		return c7result_err(std::move(res), "Cannot load element of tuple");
	    }
	    return load_elements_impl<I+1>(lxr, t);
	}
    }

    c7::result<> dump_elements(std::ostream& o, dump_context& dc,
			       const char *head_pref, const char *next_pref) const {
	return dump_elements_impl<0>(o, dc, head_pref, next_pref);
    }

    template <size_t I>
    c7::result<> dump_elements_impl(std::ostream& o, dump_context& dc,
				    const char *head_pref, const char *next_pref) const {
	if constexpr (I == size()) {
	    return c7result_ok();
	} else {
	    if constexpr (I == 0) {
		o << head_pref;
	    } else {
		o << next_pref;
	    }
	    if (auto res = get<I>().dump(o, dc); !res) {
		return res;
	    }
	    return dump_elements_impl<I+1>(o, dc, head_pref, next_pref);
	}
    }
};


template <typename... Proxies>
bool operator==(const proxy_tuple<Proxies...>& a,
		const proxy_tuple<Proxies...>& b)
{
    return a() == b();
}

template <typename... Proxies>
bool operator!=(const proxy_tuple<Proxies...>& a,
		const proxy_tuple<Proxies...>& b)
{
    return a() != b();
}


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


template <typename Proxy>
bool operator==(const proxy_array<Proxy>& a, const proxy_array<Proxy>& b)
{
    return a() == b();
}

template <typename Proxy>
bool operator!=(const proxy_array<Proxy>& a, const proxy_array<Proxy>& b)
{
    return a() != b();
}


/*----------------------------------------------------------------------------
                                  proxy_dict
----------------------------------------------------------------------------*/

class proxy_dict_base {
public:
    virtual ~proxy_dict_base() = default;

    c7::result<> load(lexer&, token&);
    c7::result<> dump(std::ostream& o, dump_context& dc) const;

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
    // inherit dump()

    void clear() {
	dic_.clear();
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
	if constexpr (std::is_same_v<KeyProxy, proxy_basic<std::string>> ||
		      std::is_same_v<KeyProxy, proxy_basic<time_us>> ||
		      std::is_same_v<KeyProxy, proxy_basic<binary_t>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<std::string>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<time_us>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<binary_t>>) {
	    if (auto res = k.from_str(key_str); !res) {
		return res;
	    }
	} else {
	    std::istringstream in{key_str.str()};
	    // c7::json_load ... {
	    c7::json::lexer lxr;
	    if (auto res = lxr.start(in); !res) {
		return res;
	    }
	    c7::json::token tkn;
	    lxr.get(tkn);
	    if (auto res = k.load(lxr, tkn); !res) {
		return c7result_err(std::move(res), "json_dict: key:%{}", key_str);
	    }
	    // c7::json_load ... }
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
	if constexpr (std::is_same_v<KeyProxy, proxy_basic<std::string>> ||
		      std::is_same_v<KeyProxy, proxy_basic<time_us>> ||
		      std::is_same_v<KeyProxy, proxy_basic<binary_t>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<std::string>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<time_us>> ||
		      std::is_same_v<KeyProxy, proxy_basic_strict<binary_t>>) {
	    if (auto res = k.dump(o, dc); !res) {
		return res;
	    }
	} else {
	    std::ostringstream so;
	    dump_context tmp_dc{};
	    tmp_dc.time_prec = dc.time_prec;
	    k.dump(so, tmp_dc);
	    jsonize_string(o, so.str());
	}
	o << ':';
	return v.dump(o, dc);
    }
};


template <typename KeyProxy, typename ValueProxy>
bool operator==(const proxy_dict<KeyProxy, ValueProxy>& a,
		const proxy_dict<KeyProxy, ValueProxy>& b)
{
    return a() == b();
}

template <typename KeyProxy, typename ValueProxy>
bool operator!=(const proxy_dict<KeyProxy, ValueProxy>& a,
		const proxy_dict<KeyProxy, ValueProxy>& b)
{
    return a() != b();
}


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
	std::function<c7::result<>(void *, lexer&, token&)> load;
	std::function<c7::result<>(const void *, std::ostream&, dump_context&)> dump;
	std::function<void(void *)> clear;
    };

    proxy_object() = default;

    proxy_object(const proxy_object&) = default;
    proxy_object(proxy_object&&) = default;
    proxy_object& operator=(const proxy_object&) = default;
    proxy_object& operator=(proxy_object&&) = default;

protected:
    using attr_t = std::pair<std::unordered_map<std::string, proxy_ops>&,
			     std::vector<std::string>&>;

    template <typename Derived, typename Proxy>
    std::pair<std::string, proxy_ops>
    member_attribute_(std::vector<std::string>& order,
		     const std::string& name,
		     Proxy Derived::*member) const {
	order.push_back(name);
	return std::pair<std::string, proxy_ops>{
	    name,
	    proxy_ops{
		[member](void *self, auto& lxr, auto &t) {
		    return (static_cast<Derived*>(self)->*member).load(lxr, t);
		},
		[member](const void *self, auto& out, auto &dc) {
		    return (static_cast<const Derived*>(self)->*member).dump(out, dc);
		},
		[member](void *self) {
		    (static_cast<Derived*>(self)->*member).clear();
		},
	    }
	};
    }

    c7::result<> load_impl(lexer&, token&, attr_t);
    c7::result<> dump_impl(std::ostream&, dump_context&, attr_t) const;
    void clear_impl(attr_t);

private:
    std::shared_ptr<std::unordered_map<std::string, proxy_unconcern>> unconcerns_;
    std::vector<std::string> src_order_;
};


/*----------------------------------------------------------------------------
                proxy_struct (strict version of proxy_object)
----------------------------------------------------------------------------*/

class proxy_struct {
public:
    using proxy_ops = proxy_object::proxy_ops;

    proxy_struct() = default;

    proxy_struct(const proxy_struct&) = default;
    proxy_struct(proxy_struct&&) = default;
    proxy_struct& operator=(const proxy_struct&) = default;
    proxy_struct& operator=(proxy_struct&&) = default;

protected:
    using attr_t = std::pair<std::unordered_map<std::string, proxy_ops>&,
			     std::vector<std::string>&>;

    template <typename Derived, typename Proxy>
    std::pair<std::string, proxy_ops>
    member_attribute_(std::vector<std::string>& order,
		     const std::string& name,
		     Proxy Derived::*member) const {
	order.push_back(name);
	return std::pair<std::string, proxy_ops>{
	    name,
	    proxy_ops{
		[member](void *self, auto& lxr, auto &t) {
		    return (static_cast<Derived*>(self)->*member).load(lxr, t);
		},
		[member](const void *self, auto& out, auto &dc) {
		    return (static_cast<const Derived*>(self)->*member).dump(out, dc);
		},
		[member](void *self) {
		    (static_cast<Derived*>(self)->*member).clear();
		},
	    }
	};
    }

    c7::result<> load_impl(lexer&, token&, attr_t);
    c7::result<> dump_impl(std::ostream&, dump_context&, attr_t) const;
    void clear_impl(attr_t);
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::json


namespace std {

template <typename T, typename Tag>
struct hash<c7::json::proxy_basic_strict<T, Tag>> {
    size_t operator()(const c7::json::proxy_basic_strict<T, Tag>& k) const {
	return hash<T>()(k());
    }
};

template <typename T, typename Tag>
struct hash<c7::json::proxy_basic<T, Tag>> {
    size_t operator()(const c7::json::proxy_basic<T, Tag>& k) const {
	return hash<T>()(k());
    }
};

template <typename P1, typename P2>
struct hash<c7::json::proxy_pair<P1, P2>> {
    size_t operator()(const c7::json::proxy_pair<P1, P2>& k) const {
	size_t v = 17;
	v = (31 * v) + std::hash<P1>()(k().first);
	v = (31 * v) + std::hash<P2>()(k().second);
	return v;
    }
};

template <typename... Proxies>
struct hash<c7::json::proxy_tuple<Proxies...>> {
    size_t operator()(const c7::json::proxy_tuple<Proxies...>& k) const {
	return calculate<0>(k, 17);
    }
    template <size_t I>
    static size_t calculate(const c7::json::proxy_tuple<Proxies...>& k, size_t v) {
	if constexpr (I == sizeof...(Proxies)) {
	    return v;
	} else {
	    using type = c7::typefunc::remove_cref_t<decltype(k.template get<I>())>;
	    return (31 * v) + std::hash<type>()(k.template get<I>());
	}
    }
};

} // namespace std


#endif // c7json/proxy.hpp
