/*
 * c7json.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_JSON_HPP_LOADED_
#define C7_JSON_HPP_LOADED_
#include <c7common.hpp>


#include <sstream>
#include <c7file.hpp>
#include <c7json/proxy.hpp>


/*
 * class MyJsonObject: public c7::json_object {
 *     c7::json_int id;
 *     c7::json_str name;
 *     // ...
 *
 *     c7json_init(
 *         c7json_member(id),
 *         c7json_member(name),
 *         // ...
 *     )
 * };
 */


namespace c7 {


using json_int	= c7::json::proxy_basic<int64_t>;
using json_real	= c7::json::proxy_basic<double>;
using json_bool	= c7::json::proxy_basic<bool>;
using json_str	= c7::json::proxy_basic<std::string>;
using json_usec	= c7::json::proxy_basic<c7::json::time_us>;	// c7::usec_t
using json_bin	= c7::json::proxy_basic<std::vector<uint8_t>>;

template <typename Tag>
using json_tagged_int = c7::json::proxy_basic_strict<int64_t, Tag>;

template <typename Tag>
using json_tagged_str = c7::json::proxy_basic_strict<std::string, Tag>;

template <typename Proxy1, typename Proxy2>
using json_pair = c7::json::proxy_pair<Proxy1, Proxy2>;

template <typename... Proxies>
using json_tuple = c7::json::proxy_tuple<Proxies...>;

template <typename Proxy>
using json_array = c7::json::proxy_array<Proxy>;

template <typename KeyProxy, typename ValueProxy>
using json_dict = c7::json::proxy_dict<KeyProxy, ValueProxy>;

using json_struct = c7::json::proxy_struct;

using json_strict = c7::json::proxy_strict;

using json_object = c7::json::proxy_object;


template <typename JsonProxy>
c7::result<> json_load(JsonProxy& proxy, std::istream& in)
{
    c7::json::lexer lxr;
    if (auto res = lxr.start(in); !res) {
	return res;
    }
    c7::json::token tkn;
    if (lxr.get(tkn) == c7::json::TKN_none) {
	return c7result_err(ENODATA);
    }
    proxy.clear();
    return proxy.load(lxr, tkn);
}


template <typename JsonProxy>
c7::result<> json_load(JsonProxy& proxy, const std::string& path)
{
    size_t size = 0;
    if (auto res = c7::file::read<char>(path, size); !res) {
	return res.as_error();
    } else {
	auto top = std::move(res.value());
	std::string buf{top.get(), size};
	std::istringstream iss{buf};
	return json_load(proxy, iss);
    }
}


template <typename JsonProxy>
c7::result<> json_dump(JsonProxy& proxy, std::ostream& o, int indent = 0)
{
    c7::json::dump_helper dh{};
    dh.context.indent = indent;
    return proxy.dump(o, dh);
}


template <typename JsonProxy>
c7::result<> json_dump(JsonProxy& proxy, std::ostream& o, c7::json::dump_context& dc)
{
    c7::json::dump_helper dh{dc};
    return proxy.dump(o, dh);
}


template <typename JsonProxy>
c7::result<> json_dump(JsonProxy& proxy, const std::string& path, int indent = 0)
{
    std::ostringstream oss;
    if (auto res = json_dump(proxy, oss, indent); !res) {
	return res;
    }
    if (indent) {
	oss << '\n';
    }
    return c7::file::rewrite(path, oss.str().data(), oss.str().size(), ".old");
}


template <typename JsonProxy>
c7::result<> json_dump(JsonProxy& proxy, const std::string& path, c7::json::dump_context& dc)
{
    std::ostringstream oss;
    if (auto res = json_dump(proxy, oss, dc); !res) {
	return res;
    }
    if (dc.indent) {
	oss << '\n';
    }
    return c7::file::rewrite(path, oss.str().data(), oss.str().size(), ".old");
}


} // namespace c7


#endif // c7json.hpp
