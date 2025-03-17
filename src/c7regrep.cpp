/*
 * c7regrep.cpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <cctype>
#include <cstring>
#include <c7app.hpp>
#include <c7regrep.hpp>
#include <c7string.hpp>


namespace c7 {


class regrep::impl {
public:
    c7::result<> init(const std::string& reg_pat,
		      const std::string& rep_rule,
		      std::regex::flag_type regex_flag,
		      uint32_t regrep_flag);

    c7::result<bool> exec(const char *in, std::string& out);

private:
    std::regex reg_;
    std::cmatch match_;			// match_result
    std::string rep_rule_;
    uint32_t regrep_flag_;

    const char *replbeg_;
    const char *replend_;
    void (impl::*replacer_)(std::string& out);

    void replace_oldrule(std::string& out);
    void replace_newrule(std::string& out);
    c7::result<const char*> translate(std::string& out,
				      const char *vn,
				      bool enclosed);
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

static constexpr const char * const digits = "0123456789";

c7::result<>
regrep::impl::init(const std::string& reg_pat,
		   const std::string& rep_rule,
		   std::regex::flag_type regex_flag,
		   uint32_t regrep_flag)
{
    if ((regrep_flag & C7_REGREP_OLDRULE) == 0 &&
	(regrep_flag & C7_REGREP_EVAL_CBSS) != 0) {
	rep_rule_ = c7::str::eval_C(rep_rule);
    } else {
	rep_rule_ = rep_rule;
    }

    if ((regrep_flag & C7_REGREP_RULEONLY) != 0) {
	replbeg_ = replend_ = "\n";		// 'replbeg =' is for old gcc
    } else if ((regrep_flag & C7_REGREP_HIGHLIGHT) != 0) {
	replbeg_ = "\x1b[31m";
	replend_ = "\x1b[0m";
    } else {
	replbeg_ = replend_ = "";
    }

    if ((regrep_flag & C7_REGREP_OLDRULE) != 0) {
	replacer_ = &impl::replace_oldrule;
    } else {
	replacer_ = &impl::replace_newrule;
    }

    regrep_flag_ = regrep_flag;

    regex_flag &= (~std::regex::nosubs);

    try {
	reg_ = std::regex(reg_pat, regex_flag);
    } catch (std::regex_error& e) {
	return c7result_err(e.what());
    }

    return c7result_ok();
}


void
regrep::impl::replace_oldrule(std::string& out)
{
    const char *p;
    const char *rule = rep_rule_.c_str();

    while ((p = std::strchr(rule, '\\')) != nullptr) {
	out.append(rule, p);
	if (p[1] == '\\') {
	    out += '\\';
	    rule = p + 2;
	} else if (auto d = std::strchr(digits, p[1]); d != nullptr) {
	    int match_index = d - digits;
	    const auto& sub = match_[match_index];
	    if (sub.matched) {
		out += sub.str();
	    }
	    rule = p + 2;
	} else {
	    out += '\\';
	    rule = p + 1;
	}
    }
    out += rule;
}


c7::result<const char*>
regrep::impl::translate(std::string& out, const char *vn, bool enclosed)
{
    std::string& (*copy)(std::string&, const std::string&) = nullptr;

    int match_index;
    if (auto d = std::strchr(digits, *vn); d == nullptr) {
	return c7result_err("invalid sub-match reference '%{}'", vn);
    } else {
	match_index = d - digits;
    }
    auto mod = vn + 1;

    if (enclosed) {
	if (std::strcmp(mod, ":L") == 0) {
	    copy = c7::str::lower<std::string>;
	    mod += 2;
	} else if (std::strcmp(mod, ":U") == 0) {
	    copy = c7::str::upper<std::string>;
	    mod += 2;
	}
	if (*mod != 0) {
	    return c7result_err("invalid sub-match reference '%{}'", vn);
	}
    }
    vn = mod;

    const auto& sub = match_[match_index];
    if (sub.matched) {
	if (copy == nullptr) {
	    out += sub.str();
	} else {
	    copy(out, sub.str());
	}
    }
    return c7result_ok(vn);
}


void
regrep::impl::replace_newrule(std::string& out)
{
    c7::str::eval(out, rep_rule_.c_str(), '%', '\\',
		  [this](auto& o, auto vn, auto e) {
		      return translate(o, vn, e);
		  });
}


c7::result<bool>
regrep::impl::exec(const char *in, std::string& out)
{
    bool replaced = false;
    bool ruleonly = ((regrep_flag_ & C7_REGREP_RULEONLY) != 0);

    try {
	for (;;) {
	    if (!std::regex_search(in, match_, reg_)) {
		if (!ruleonly) {
		    out += in;
		}
		return c7result_ok(replaced);
	    }
	    replaced = true;
	    if (!ruleonly) {
		out += match_.prefix();
	    }
	    out += replbeg_;
	    (this->*replacer_)(out);
	    out += replend_;
	    in += (match_.prefix().length() + match_[0].length());
	}
    } catch (std::regex_error& e) {
	return c7result_err(e.what());
    }
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

regrep::regrep(): pimpl_(new impl)
{
}

regrep::~regrep() = default;

c7::result<>
regrep::init(const std::string& reg_pat,
	     const std::string& rep_rule,
	     std::regex::flag_type regex_flag,
	     uint32_t regrep_flag)
{
    return pimpl_->init(reg_pat, rep_rule, regex_flag, regrep_flag);
}

c7::result<bool>
regrep::exec(const char *in, std::string& out)
{
    return pimpl_->exec(in, out);
}


} // namespace c7
