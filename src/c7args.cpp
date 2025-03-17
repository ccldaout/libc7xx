/*
 * c7args.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <ctime>
#include <map>
#include <unordered_map>
#include <c7app.hpp>
#include <c7args.hpp>
#include <c7string.hpp>
#include <c7nseq/flat.hpp>
#include <c7nseq/string.hpp>
#include <c7nseq/transform.hpp>


namespace c7::args {


void print_type(std::ostream& o, const std::string&, opt_desc::prm_type t)
{
    const char *s = nullptr;
    switch (t) {
    case opt_desc::prm_type::BOOL:	s = "BOOL"; break;
    case opt_desc::prm_type::INT:	s = "INT" ; break;
    case opt_desc::prm_type::UINT:	s = "UINT"; break;
    case opt_desc::prm_type::REAL:	s = "REAL"; break;
    case opt_desc::prm_type::KEY:	s = "KEY" ; break;
    case opt_desc::prm_type::REG:	s = "REG" ; break;
    case opt_desc::prm_type::DATE:	s = "DATE"; break;
    case opt_desc::prm_type::ANY:	s = "ANY" ; break;
    default:					    break;
    }
    if (s) {
	o << s;
    } else {
	c7::format(o, "?(%{})", t);
    }
}


void print_type(std::ostream& o, const std::string&, const opt_desc& d)
{
    c7::format(o, "desc<L:%{}, S:%{}, %{}, #%{}:%{}, keys:{%{, }}>",
	       d.long_name, d.short_name, d.type, d.prmc_min, d.prmc_max, d.keys);
}


void print_type(std::ostream& o, const std::string&, const opt_value& v)
{
    c7::format(o, "value<type:%{}, param:'%{}', key_index:%{}, ",
	       v.type, v.param, v.key_index);
    switch (v.type) {
    case opt_desc::prm_type::BOOL: c7::format(o, "b:%{}, ", v.b);	break;
    case opt_desc::prm_type::INT:  c7::format(o, "i:%{}, ", v.i);	break;
    case opt_desc::prm_type::UINT: c7::format(o, "u:%{}, ", v.u);	break;
    case opt_desc::prm_type::REAL: c7::format(o, "d:%{}, ", v.d);	break;
    case opt_desc::prm_type::DATE: c7::format(o, "t:%{T%Y %m/%d %H:%M.%S}, ", v.t);	break;
    default:					    			break;
    }
    c7::format(o, "match:%{ }>",
	       v.regmatch
	       | c7::nseq::transform([](auto&& s){ return "{"+s+"}"; })
	       | c7::nseq::to_vector());
}


static c7::result<>
init_regs(const std::vector<std::string>& reg_strv,
	  std::vector<std::regex>& regs,
	  bool ignore_case)
{
    auto flags = std::regex::ECMAScript;
    if (ignore_case) {
	flags |= std::regex::icase;
    }
    regs.clear();
    for (auto& s: reg_strv) {
	try {
	    regs.emplace_back(s, flags);
	} catch (std::regex_error& e) {
	    return c7result_err("invalid regex '%{}'\n%{}", s, e.what());
	}
    }
    return c7result_ok();
}


static c7::result<int>
check_regs(const std::vector<std::regex>& regs,
	   const std::string& s,
	   const opt_desc& d,
	   c7::strvec& match)
{
    std::smatch m;
    match.clear();
    try {
	for (size_t i = 0; i < regs.size(); i++) {
	    auto& reg = regs[i];
	    if (std::regex_match(s, m, reg)) {
		for (size_t i = 0; i < m.size(); i++) {
		    if (m[i].matched) {
			match.push_back(m[i].str());
		    } else {
			match.push_back("");
		    }
		}
		return c7result_ok(static_cast<int>(i));
	    }
	}
    } catch (std::exception& e) {
	return c7result_err(EFAULT, "regex_match failed: %{}", e.what());
    }
    return c7result_err(EINVAL,
			"Specified parameter '%{}' as %{} don't match any patterns as follow:\n%{, }.",
			s, d.prm_name,
			d.reg_descrips.empty() ? d.keys : d.reg_descrips);
}


/*----------------------------------------------------------------------------
                                 opt_handler
----------------------------------------------------------------------------*/

struct opt_context {
    opt_desc desc;
    std::vector<opt_value> vals;
    parser::callback_obj callback;
};


class opt_handler {
public:
    opt_handler(const opt_desc& desc, parser::callback_obj callback):
	ctx_({desc, {}, callback}) {
    }
    virtual ~opt_handler() {}
    virtual c7::result<> init() = 0;
    virtual c7::result<> parse(c7::strvec&& prms) = 0;
    unsigned int prmc_min() const { return ctx_.desc.prmc_min; }
    unsigned int prmc_max() const { return ctx_.desc.prmc_max; }
    const opt_desc& desc() const { return ctx_.desc; }
    void clear_value() { ctx_.vals.clear(); }

protected:
    opt_context ctx_;
};


class bool_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	auto& d = ctx_.desc;
	d.keys.push_back("(n|no|f|false|0)");
	d.keys.push_back("(y|yes|t|true|1)");
	return init_regs(d.keys, regs_, true);
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    opt_value v{d.type, s};
	    if (auto res = check_regs(regs_, s, d, v.regmatch); !res) {
		return res.as_error();
	    } else {
		v.key_index = res.value();
	    }
	    v.b = (v.key_index == 1);
	    ctx_.vals.push_back(std::move(v));
	}
	return ctx_.callback(d, ctx_.vals);
    }

private:
    std::vector<std::regex> regs_;
};


template <typename Numeric>
class numeric_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	return c7result_ok();
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    opt_value v{d.type, s};
	    char *end;
	    if constexpr (std::is_same_v<Numeric, int64_t>) {
		v.i = std::strtol(s.c_str(), &end, 0);
	    } else if constexpr (std::is_same_v<Numeric, uint64_t>) {
		v.u = std::strtoul(s.c_str(), &end, 0);
	    } else if constexpr (std::is_same_v<Numeric, double>) {
		v.d = std::strtod(s.c_str(), &end);
	    } else {
		static_assert(c7::typefunc::false_v<Numeric>,
			      "Numeric must be one of int64_t, uint64_t, double.");
	    }
	    if (*end != 0) {
		return c7result_err(EINVAL,
				    "Specified parameter '%{}' as %{} include invalid character '%{}'.",
				    s, d.prm_name, *end);
	    }
	    ctx_.vals.push_back(std::move(v));
	}
	return ctx_.callback(d, ctx_.vals);
    }
};


using  int_handler = numeric_handler<int64_t>;
using uint_handler = numeric_handler<uint64_t>;
using real_handler = numeric_handler<double>;


class key_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	return c7result_ok();
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    opt_value v{d.type, s};
	    if (d.ignore_case) {
		s.clear();
		c7::str::lower(s, v.param);
	    }
	    v.key_index = c7::strmatch<std::string>(s, d.keys);
	    if (v.key_index == -1) {
		return c7result_err(EINVAL,
				    "Specified parameter '%{}' as %{} don't match any keywords as follow:\n%{, }.",
				    v.param, d.prm_name, d.keys);
	    }
	    ctx_.vals.push_back(std::move(v));
	}
	return ctx_.callback(d, ctx_.vals);
    }
};


class reg_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	auto& d = ctx_.desc;
	return init_regs(d.keys, regs_, d.ignore_case);
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    opt_value v{d.type, s};
	    if (auto res = check_regs(regs_, s, d, v.regmatch); !res) {
		return res.as_error();
	    } else {
		v.key_index = res.value();
	    }
	    ctx_.vals.push_back(std::move(v));
	}
	return ctx_.callback(d, ctx_.vals);
    }

private:
    std::vector<std::regex> regs_;
};


class date_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	auto& d = ctx_.desc;
	d.keys.push_back(R"(((\d\d)?(\d\d)(\d\d))?(\d\d)(\d\d)(\.(\d\d))?)");
	d.reg_descrips.push_back("[[YY]MMDD]hhmm[.ss]");
	return init_regs(d.keys, regs_, false);
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    opt_value v{d.type, s};
	    if (auto res = check_regs(regs_, s, d, v.regmatch); !res) {
		return res.as_error();
	    } else {
		v.key_index = res.value();
		try {
		    v.t = get_time(v.regmatch);
		} catch (c7::result_exception& e) {
		    return e.as_result();
		}
	    }
	    ctx_.vals.push_back(std::move(v));
	}
	return ctx_.callback(d, ctx_.vals);
    }

private:
    std::vector<std::regex> regs_;

    static c7::result<int>
    tm_item(const std::string& s, int min, int max, const std::string& name) {
	char *p;
	int i = std::strtol(s.c_str(), &p, 10);
	if (i < min || i > max) {
	    return c7result_err(ERANGE, "Specified %{} as %{} is out of range.",
				s, name);
	}
	return c7result_ok(i);
    }

    static std::time_t
    get_time(const c7::strvec& match) {
	std::time_t tv = std::time(nullptr);
	std::tm tm;
	localtime_r(&tv, &tm);

	// match[2] : YY
	// match[3] : MM
	// match[4] : DD
	// match[5] : hh
	// match[6] : mm
	// match[8] : ss

	if (auto s = match[2]; !s.empty()) {
	    tm.tm_year = tm_item(s, 0, 99, "YEAR").value() + 2000 - 1900;
	}
	if (auto s = match[3]; !s.empty()) {
	    tm.tm_mon  = tm_item(s, 1, 12, "MONTH").value() - 1;
	}
	if (auto s = match[4]; !s.empty()) {
	    tm.tm_mday  = tm_item(s, 1, 31, "DAY").value();
	}
	if (auto s = match[5]; !s.empty()) {
	    tm.tm_hour  = tm_item(s, 0, 23, "HOUR").value();
	}
	if (auto s = match[6]; !s.empty()) {
	    tm.tm_min  = tm_item(s, 0, 59, "MIN").value();
	}
	if (auto s = match[8]; !s.empty()) {
	    tm.tm_sec  = tm_item(s, 0, 61, "SEC").value();
	}

	return std::mktime(&tm);
    }
};


class any_handler: public opt_handler {
public:
    using opt_handler::opt_handler;

    c7::result<> init() override {
	return c7result_ok();
    }

    c7::result<> parse(c7::strvec&& prms) override {
	const auto& d = ctx_.desc;
	for (auto& s: prms) {
	    ctx_.vals.push_back(opt_value{d.type, s});
	}
	return ctx_.callback(d, ctx_.vals);
    }
};


static std::shared_ptr<opt_handler>
make(const opt_desc& desc, parser::callback_obj callback)
{
    switch (desc.type) {
    case opt_desc::prm_type::BOOL:	return std::make_shared<bool_handler>(desc, callback);
    case opt_desc::prm_type::INT:	return std::make_shared< int_handler>(desc, callback);
    case opt_desc::prm_type::UINT:	return std::make_shared<uint_handler>(desc, callback);
    case opt_desc::prm_type::REAL:	return std::make_shared<real_handler>(desc, callback);
    case opt_desc::prm_type::KEY:	return std::make_shared< key_handler>(desc, callback);
    case opt_desc::prm_type::REG:	return std::make_shared< reg_handler>(desc, callback);
    case opt_desc::prm_type::DATE:	return std::make_shared<date_handler>(desc, callback);
    case opt_desc::prm_type::ANY:	return std::make_shared< any_handler>(desc, callback);
    default:				return nullptr;
    }
}


/*----------------------------------------------------------------------------
                                 parser::impl
----------------------------------------------------------------------------*/

class parser::impl {
public:
    c7::result<> add_opt(const opt_desc&, callback_obj);
    c7::result<> prepare();
    c7::result<char**> parse(char **argv);
    void append_usage(c7::strvec& usage, size_t main_indent, size_t descrip_indent);

private:
    std::unordered_map<std::string,
		       std::shared_ptr<opt_handler>> dic_;
    c7::strvec opt_names_;
    std::string noprm_singles_;
    std::string n_prm_singles_;

    std::string find_option(const char * const arg);
};


// add_opt

static c7::result<>
check_long_name(const opt_desc& desc)
{
    static std::regex rule{"[a-zA-Z][a-zA-Z0-9_-]*[a-zA-Z0-9]"};
    if (std::regex_match(desc.long_name, rule)) {
	return c7result_ok();
    } else {
	return c7result_err(EINVAL, "long option name has invalid character: %{}", desc);
    }
}


static c7::result<>
check_short_name(const opt_desc& desc)
{
    static std::regex rule{"[a-zA-Z0-9]"};
    if (std::regex_match(desc.short_name, rule)) {
	return c7result_ok();
    } else {
	return c7result_err(EINVAL, "short option name has invalid character: %{}", desc);
    }
}


c7::result<>
parser::impl::add_opt(const opt_desc& desc, callback_obj callback)
{
    if (desc.long_name.empty() && desc.short_name.empty()) {
	return c7result_err(EINVAL, "both long option name and short option name are empty:\n%{}", desc);
    }

    auto handler = make(desc, callback);
    if (handler == nullptr) {
	return c7result_err(EINVAL, "Invalid prm_type is specified:\n%{}", desc);
    }

    if (desc.prmc_min > desc.prmc_max) {
	return c7result_err(EINVAL, "Invalid prmc_min > prmc_max:\n%{}", desc);
    }

    if (desc.type == opt_desc::prm_type::KEY || desc.type == opt_desc::prm_type::REG) {
	if (desc.keys.empty()) {
	    return c7result_err(EINVAL, "empty keys is not avilable for KEY or REG type:\n%{}", desc);
	}
    } else {
	if (!desc.keys.empty()) {
	    return c7result_err(EINVAL, "keys is not available for %{} type:\n%{}", desc.type, desc);
	}
    }

    if (desc.type == opt_desc::prm_type::REG) {
	if (!desc.reg_descrips.empty() &&
	    desc.reg_descrips.size() != desc.keys.size()) {
	    return c7result_err(EINVAL, "count of reg_descrips is different from one of keys:\n%{}", desc);
	}
    }

    if (!desc.long_name.empty()) {
	if (auto res = check_long_name(desc); !res) {
	    return res;
	}
	auto name = "--" + desc.long_name;
	if (auto [_, ins] = dic_.try_emplace(name, handler); !ins) {
	    return c7result_err(EINVAL, "long option is duplicated:\n%{}", desc);
	}
	opt_names_.push_back(std::move(name));
    }

    if (!desc.short_name.empty()) {
	if (auto res = check_short_name(desc); !res) {
	    return res;
	}
	auto name = "-" + desc.short_name;
	if (auto [_, ins] = dic_.try_emplace(name, handler); !ins) {
	    return c7result_err(EINVAL, "short option is duplicated:\n%{}", desc);
	}
	opt_names_.push_back(std::move(name));
	if (desc.prmc_max == 0) {
	    noprm_singles_ += desc.short_name[0];
	}
	if (desc.prmc_max > 0) {
	    n_prm_singles_ += desc.short_name[0];
	}
    }

    return handler->init();
}


// parser

std::string
parser::impl::find_option(const char * const arg)
{
    for (const auto& s: opt_names_) {
	if (std::strncmp(arg, s.c_str(), s.size()) == 0) {
	    if (arg[s.size()] == 0 || arg[s.size()] == '=') {
		return s;
	    }
	    if (s.size() == 2 && n_prm_singles_.find(s[1]) != std::string::npos) {
		return s;
	    }
	}
    }
    return "";
}


c7::result<char**>
parser::impl::parse(char **argv)
{
    for (; *argv; argv++) {
	const char * const arg = *argv;
	if (arg[0] != '-') {
	    break;
	}
	if (arg[1] == 0 || (arg[1] == '-' && arg[2] == 0)) {
	    argv++;
	    break;
	}

	if (auto s = find_option(arg); !s.empty()) {
	    auto it = dic_.find(s);
	    if (it == dic_.end()) {
		return c7result_err(EFAULT, "[bug] Unexpected no option handler: %{}", s);
	    }
	    auto& [_, handler] = *it;

	    const char *prm = nullptr;
	    if (arg[s.size()] == '=') {
		// --long=PRM
		// -s=PRM
		if (handler->prmc_max() == 0) {
		    return c7result_err(EINVAL, "Unexpected paramter '%{}' is specified:\n%{}",
					&arg[s.size()], handler->desc());
		}
		prm = &arg[s.size()+1];
	    } else if (arg[s.size()] == 0) {
		// --long PRM or --long
		//  -s    PRM or  -s
		if (handler->prmc_min() > 0) {
		    prm = *(++argv);
		}
	    } else {
		// -sPRM
		prm = &arg[s.size()];
	    }

	    c7::strvec prms;
	    if (prm) {
		if (handler->prmc_max() > 1) {
		    prms = c7::str::split(prm, ',');
		} else {
		    prms += prm;
		}
	    }
	    if (prms.size() < handler->prmc_min()) {
		return c7result_err(EINVAL, "Too few parameters '%{}':\n%{}",
				    prm, handler->desc());
	    }
	    if (prms.size() > handler->prmc_max()) {
		return c7result_err(EINVAL, "Too many parameters '%{}':\n%{}",
				    prm, handler->desc());
	    }

	    handler->clear_value();
	    if (auto res = handler->parse(std::move(prms)); !res) {
		return res.as_error();
	    }
	} else {
	    // -abc

	    if (arg[1] == '-') {
		return c7result_err(EINVAL, "Invalid long option name '%{}'", arg);
	    }

	    // special singles
	    for (auto p = arg+1; *p; p++) {
		if (noprm_singles_.find(*p) == std::string::npos) {
		    return c7result_err(EINVAL, "Invalid short option character '%{}'", *p);
		}
		std::string s{"--"};
		s[1] = *p;
		auto it = dic_.find(s);
		if (it == dic_.end()) {
		    return c7result_err(EFAULT, "[bug] Unexpected no option handler: %{}", s);
		}
		auto& [_, handler] = *it;
		handler->clear_value();
		if (auto res = handler->parse(c7::strvec{}); !res) {
		    return res.as_error();
		}
	    }
	}
    }

    return c7result_ok(argv);
}


// usage

static void
append_descrip(c7::strvec& usage, const opt_desc& d, const std::string& pref, size_t descrip_off)
{
    // -s, --long_name     <opt_descrip>
    // -s, --long-long-long-name
    //                     <opt_descrip>
    // -s, --long-name=PRM_NAME			min:1, max:1
    // -s, --long-name[=PRM_NAME]		min:0, max:1
    // -s, --long-name=PRM_NAME,PRM_NAME	min:2, max:2
    // -s, --long-name[=PRM_NAME,...]		min:0, max>1
    // -s, --long-name=PRM_NAME[,PRM_NAME]	min:1, max:2
    // -s, --long-name=PRM_NAME[,...]		min:1, max>2
    //                     <opt_descrip>
    //                     PRM_NAME: <prm_descrip>
    //                             : ..., ....  [KEY]
    std::string s;

    // option names

    if (d.short_name.empty()) {
	s += "    ";
    } else {
	s += c7::format("-%{}, ", d.short_name[0]);
    }
    if (!d.long_name.empty()) {
	s += "--";
	s += d.long_name;
    }

    // parameter part

    if (d.prmc_max > 0) {
	if (       d.prmc_min == 0 && d.prmc_max == 1) {
	    s += c7::format("[=%{}]", d.prm_name);
	} else if (d.prmc_min == 0 && d.prmc_max == 2) {
	    s += c7::format("[=%{}[,%{}]]", d.prm_name, d.prm_name);
	} else if (d.prmc_min == 0 && d.prmc_max >  2) {
	    s += c7::format("[=%{},...]", d.prm_name);
	} else if (d.prmc_min == 1 && d.prmc_max == 1) {
	    s += c7::format("=%{}", d.prm_name);
	} else if (d.prmc_min == 1 && d.prmc_max == 2) {
	    s += c7::format("=%{}[,%{}]", d.prm_name, d.prm_name);
	} else if (d.prmc_min == 1 && d.prmc_max >  2) {
	    s += c7::format("=%{}[,...]", d.prm_name);
	} else if (d.prmc_min == 2 && d.prmc_max == 2) {
	    s += c7::format("=%{},%{}", d.prm_name, d.prm_name);
	} else {
	    s += c7::format("=%{},%{},...", d.prm_name, d.prm_name);
	}
    }

    // option description

    if (s.size() >= descrip_off) {
	usage += (pref + std::move(s));
	s.append(descrip_off, ' ');
    } else {
	s.append(descrip_off - s.size(), ' ');
    }
    s += d.opt_descrip;
    usage += (pref + std::move(s));

    // parameter description

    if (d.prmc_max > 0) {
	s.append(descrip_off, ' ');
	s += d.prm_name;
	auto n = s.size();
	s += ": ";
	s += d.prm_descrip;
	usage += (pref + std::move(s));

	if (d.type == opt_desc::prm_type::BOOL ||
	    d.type == opt_desc::prm_type::KEY ||
	    d.type == opt_desc::prm_type::DATE) {
	    s.append(n, ' ');
	    s += ": ";
	    if (d.type == opt_desc::prm_type::DATE) {
		s += d.reg_descrips[0];
	    } else {	// if (d.type == opt_desc::prm_type::BOOL ||
			//     d.type == opt_desc::prm_type::KEY) {
		s += c7::format("%{,}", d.keys);
	    }
	    usage += (pref + std::move(s));
	}
    }
}

void
parser::impl::append_usage(c7::strvec& usage, size_t main_indent, size_t descrip_indent)
{
    std::map<std::string, std::shared_ptr<opt_handler>> dic;
    for (auto& [_, h]: dic_) {
	auto& d = h->desc();
	auto s = d.short_name.empty() ? d.long_name : d.short_name;
	dic.emplace(std::move(s), h);
    }

    std::string pref;
    pref.append(main_indent, ' ');

    auto off = descrip_indent - main_indent;
    for (auto& [_, h]: dic) {
	append_descrip(usage, h->desc(), pref, off);
    }
}


/*----------------------------------------------------------------------------
                                    parser
----------------------------------------------------------------------------*/

parser::parser(): pimpl_(new impl) {}


parser::~parser() = default;


c7::result<>
parser::add_opt(const opt_desc& desc, callback_obj callback)
{
    return pimpl_->add_opt(desc, callback);
}


c7::result<char**>
parser::parse(char **argv)
{
    return pimpl_->parse(argv);
}


void
parser::append_usage(c7::strvec& usage, size_t base_indent, size_t descrip_indent) const
{
    return pimpl_->append_usage(usage, base_indent, descrip_indent);
}


} // namespace c7::args
