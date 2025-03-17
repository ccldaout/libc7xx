/*
 * c7regrep.cpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <sys/stat.h>
#include <c7app.hpp>
#include <c7args.hpp>
#include <c7file.hpp>
#include <c7regrep.hpp>
#include <c7string.hpp>


using c7::p_;
using c7::P_;


enum regrep_result_t {
    RESULT_KEEP,
    RESULT_REPLACE,
    RESULT_ERROR,
};


struct ctx_t {
    std::regex::flag_type regex_flag = std::regex::basic;
    uint32_t regrep_flag = C7_REGREP_HIGHLIGHT;
    const char *pattern = nullptr;
    const char *rule = nullptr;
    char **files = nullptr;
    bool progress = false;
    regrep_result_t (*replace)(const char *, const c7::strvec&, ctx_t&);
    std::unique_ptr<std::regex> filter_regex;
    c7::regrep regrep;
};


/*----------------------------------------------------------------------------
                                  check mode
----------------------------------------------------------------------------*/

static regrep_result_t
repl_checkonly(const char *source, const c7::strvec& lines, ctx_t& ctx)
{
    bool putpath = false;

    std::string out;

    int line_num = 0;
    for (auto& s: lines) {
	line_num++;
	if (ctx.filter_regex != nullptr && !std::regex_search(s, *ctx.filter_regex)) {
	    continue;
	}
	out.clear();
	if (auto res = ctx.regrep.exec(s, out); res && res.value()) {
	    if (!putpath) {
		putpath = true;
		p_("%{} ...", source);
	    }
	    p_("%{7} %{}", line_num, out);
	}
    }

    return RESULT_KEEP;
}


/*----------------------------------------------------------------------------
                                  edit mode
----------------------------------------------------------------------------*/

static regrep_result_t
replace_all(std::string& out, const c7::strvec& lines, ctx_t& ctx)
{
    bool replaced = false;

    for (auto& s: lines) {
	if (ctx.filter_regex != nullptr && !std::regex_search(s, *ctx.filter_regex)) {
	    out += s;
	    continue;
	}
	if (auto res = ctx.regrep.exec(s, out); res && res.value()) {
	    replaced = true;
	}
	out += '\n';
    }
    return replaced ? RESULT_REPLACE : RESULT_KEEP;
}

// inline (no backup)
static regrep_result_t
repl_inline(const char *source, const c7::strvec& lines, ctx_t& ctx)
{
    std::string out;
    auto rep_res = replace_all(out, lines, ctx);
    if (rep_res == RESULT_ERROR) {
	return RESULT_ERROR;
    }
    if (auto res = c7::file::rewrite(source, out.data(), out.size()); !res) {
	c7echo(res);
	return RESULT_ERROR;
    }
    return rep_res;
}

// keep original
static regrep_result_t
repl_keep(const char *source, const c7::strvec& lines, ctx_t& ctx)
{
    std::string out;
    auto rep_res = replace_all(out, lines, ctx);
    if (rep_res == RESULT_ERROR) {
	return RESULT_ERROR;
    }
    std::string path = source;
    path += ".rep";
    if (auto res = c7::file::write(path, 0644, out.c_str(), out.size()); !res) {
	c7echo(res);
	return RESULT_ERROR;
    }
    c7::file::chstat(path, source);
    return rep_res;
}

// copy original with other name
static regrep_result_t
repl_copy(const char *source, const c7::strvec& lines, ctx_t& ctx)
{
    std::string out;
    auto rep_res = replace_all(out, lines, ctx);
    if (rep_res == RESULT_ERROR) {
	return RESULT_ERROR;
    }
    if (auto res = c7::file::rewrite(source, out.data(), out.size(), ".bck"); !res) {
	c7echo(res);
	return RESULT_ERROR;
    }
    return rep_res;
}

// print to stdout
static regrep_result_t repl_stdout(const char *source, const c7::strvec& lines, ctx_t& ctx)
{
    std::string out;
    auto rep_res = replace_all(out, lines, ctx);
    if (rep_res == RESULT_ERROR) {
	return RESULT_ERROR;
    }
    P_("%{}", out);
    return rep_res;
}


/*----------------------------------------------------------------------------
                                  parse argv
----------------------------------------------------------------------------*/

class parser: public c7::args::parser {
public:
    using opt_desc  = c7::args::opt_desc;
    using opt_value = c7::args::opt_value;
    using prm_type  = opt_desc::prm_type;

    explicit parser(ctx_t& ctx): ctx_(ctx) {}

    c7::result<> init() override;
    void help();

    callback_t opt_inline;
    callback_t opt_keep;
    callback_t opt_copy;
    callback_t opt_stdout;
    callback_t opt_extended;
    callback_t opt_highlight;
    callback_t opt_eval_c_bss;
    callback_t opt_oldrule;
    callback_t opt_ruleonly;
    callback_t opt_filter;
    callback_t opt_help;

private:
    ctx_t& ctx_;
};

c7::result<> parser::init()
{
    c7::result<> res;
    {
	opt_desc d;
	d.long_name = "inline";
	d.short_name = "i";
	d.opt_descrip = "replace original without backup";
	res << add_opt(d, &parser::opt_inline);
    }
    {
	opt_desc d;
	d.long_name = "keep";
	d.short_name = "k";
	d.opt_descrip = "keep original";
	res << add_opt(d, &parser::opt_keep);
    }
    {
	opt_desc d;
	d.long_name = "copy";
	d.short_name = "c";
	d.opt_descrip = "copy original as other file";
	res << add_opt(d, &parser::opt_copy);
    }
    {
	opt_desc d;
	d.long_name = "stdout";
	d.short_name = "s";
	d.opt_descrip = "print replaced contents";
	res << add_opt(d, &parser::opt_stdout);
    }
    {
	opt_desc d;
	d.long_name = "extended";
	d.short_name = "e";
	d.opt_descrip = "extended regular expression";
	res << add_opt(d, &parser::opt_extended);
    }
    {
	opt_desc d;
	d.long_name = "highlight";
	d.short_name = "H";
	d.opt_descrip = "highlight replaced string";
	d.prm_descrip = "highlight mode (default:color)";
	d.prm_name = "HIGHLIGHT";
	d.type = prm_type::KEY;
	d.keys = c7::strvec{"none", "color"};
	d.prmc_min = 0;
	d.prmc_max = 1;
	res << add_opt(d, &parser::opt_highlight);
    }
    {
	opt_desc d;
	d.long_name = "translate-c-bss";
	d.opt_descrip = "tranclate C backslash sequence";
	res << add_opt(d, &parser::opt_eval_c_bss);
    }
    {
	opt_desc d;
	d.long_name = "old-rule";
	d.opt_descrip = "use old-style RULE";
	res << add_opt(d, &parser::opt_oldrule);
    }
    {
	opt_desc d;
	d.long_name = "rule-only";
	d.opt_descrip = "print translated RULE only";
	res << add_opt(d, &parser::opt_ruleonly);
    }
    {
	opt_desc d;
	d.long_name = "filter-regexp";
	d.short_name = "f";
	d.opt_descrip = "filter lines by additional regexp";
	d.prm_descrip = "regular expression";
	d.prm_name = "REGEXP";
	d.type = prm_type::ANY;
	d.prmc_min = 1;
	d.prmc_max = 1;
	res << add_opt(d, &parser::opt_filter);
    }
    {
	opt_desc d;
	d.long_name = "help";
	d.short_name = "h";
	d.opt_descrip = "this help";
	res << add_opt(d, &parser::opt_help);
    }
    return res;
}

c7::result<>
parser::opt_inline(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.replace = repl_inline;
    ctx_.regrep_flag &= (~C7_REGREP_HIGHLIGHT);
    ctx_.progress = true;
    return c7result_ok();
}

c7::result<>
parser::opt_keep(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.replace = repl_keep;
    ctx_.regrep_flag &= (~C7_REGREP_HIGHLIGHT);
    ctx_.progress = true;
    return c7result_ok();
}

c7::result<>
parser::opt_copy(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.replace = repl_copy;
    ctx_.regrep_flag &= (~C7_REGREP_HIGHLIGHT);
    ctx_.progress = true;
    return c7result_ok();
}

c7::result<>
parser::opt_stdout(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.replace = repl_stdout;
    ctx_.regrep_flag &= (~C7_REGREP_HIGHLIGHT);
    return c7result_ok();
}

c7::result<>
parser::opt_extended(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.regex_flag &= (~std::regex::basic);
    ctx_.regex_flag |= std::regex::ECMAScript;
    return c7result_ok();
}

c7::result<>
parser::opt_highlight(const opt_desc&, const std::vector<opt_value>& vals)
{
    if (vals.size() == 0 || vals[0].key_index == 1) {
	ctx_.regrep_flag |= C7_REGREP_HIGHLIGHT;
    } else {
	ctx_.regrep_flag &= (~C7_REGREP_HIGHLIGHT);
    }
    return c7result_ok();
}

c7::result<>
parser::opt_eval_c_bss(const opt_desc&, const std::vector<opt_value>&)
{
    if ((ctx_.regrep_flag & C7_REGREP_OLDRULE) != 0) {
	c7error("--old-rule and --translate-c-bss are mutually exclusive.");
    }
    ctx_.regrep_flag |= C7_REGREP_EVAL_CBSS;
    return c7result_ok();
}

c7::result<>
parser::opt_oldrule(const opt_desc&c, const std::vector<opt_value>&)
{
    if ((ctx_.regrep_flag & C7_REGREP_EVAL_CBSS) != 0) {
	c7error("--old-rule and --translate-c-bss are mutually exclusive.");
    }
    ctx_.regrep_flag |= C7_REGREP_OLDRULE;
    return c7result_ok();
}

c7::result<>
parser::opt_ruleonly(const opt_desc&, const std::vector<opt_value>&)
{
    ctx_.regrep_flag |= C7_REGREP_RULEONLY;
    return c7result_ok();
}

c7::result<>
parser::opt_filter(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    auto flag = ctx_.regex_flag | std::regex::nosubs;
    try {
	ctx_.filter_regex = std::make_unique<std::regex>(vals[0].param, flag);
    } catch (std::regex_error& e) {
	c7error("invalid regex '%{}'\n%{}", vals[0].param, e.what());
    }
    return c7result_ok();
}

c7::result<>
parser::opt_help(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    c7::strvec usage;
    usage.push_back(c7::format(
			"Usage: %{} [options ...] REG_PATTERN REPLACE_RULE [file ...]\n"
			"\n"
			"options:\n",
			c7::app::progname));
    append_usage(usage, 2, 32);
    for (auto& s: usage) {
	p_("%{}", s);
    }
    std::exit(0);
   return c7result_ok();
}

static void parse_args(char **argv, ctx_t& ctx)
{
    parser prs{ctx};

    ctx.replace = repl_checkonly;

    if (auto res = prs.init(); !res) {
	c7error(res);
    }
    if (auto res = prs.parse(argv); !res) {
	c7error(res);
    } else {
	argv = res.value();
    }

    if ((ctx.regrep_flag & C7_REGREP_RULEONLY) != 0 && ctx.replace != repl_stdout) {
	c7error("rule-only option is availabe under stdout mode");
    }
    if (*argv == nullptr) {
	c7error("REG_PATTERN is not specified.");
    }
    ctx.pattern = *argv++;

    if (*argv == nullptr) {
	c7error("REPLACE_RULE is not specified.");
    }
    ctx.rule = *argv++;

    ctx.files = argv;
    if (*argv == nullptr &&
	(ctx.replace == repl_checkonly || ctx.replace == repl_stdout)) {
	static char *argv[] = { const_cast<char*>(""), nullptr };	// stdout
	ctx.files = argv;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static bool is_regular(const char *path)
{
    struct stat st;
    if (::stat(path, &st) == C7_SYSOK) {
	return (S_ISREG(st.st_mode) != 0);
    }
    return false;
}

int main(int argc, char **argv)
{
    ctx_t ctx;
    parse_args(argv+1, ctx);

    if (auto res = ctx.regrep.init(ctx.pattern, ctx.rule, ctx.regex_flag, ctx.regrep_flag); !res) {
	c7error(res);
    }

    for (argv = ctx.files; *argv; argv++) {
	if (auto res = c7::file::readlines(*argv); !res) {
	    if (is_regular(*argv)) {
		c7echo(res);
	    }
	} else {
	    c7::strvec lines{res.value()};
	    regrep_result_t rep_res = ctx.replace(*argv, lines, ctx);
	    if (ctx.progress && rep_res != RESULT_KEEP) {
		c7echo("%{} ... %{}",
		       *argv, (rep_res == RESULT_REPLACE) ? "ok" : "ERROR");
	    }
	}
    }

    return 0;
}
