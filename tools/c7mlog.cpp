/*
 * c7mlog.cpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>	// write
#include <cstring>
#include <ctime>
#include <string_view>
#include <unordered_set>
#include <c7app.hpp>
#include <c7args.hpp>
#include <c7mlog.hpp>
#include <c7nseq/flat.hpp>
#include <c7nseq/transform.hpp>
#include <c7nseq/string.hpp>


struct mlog_conf {
    c7::mlog_reader reader;

    // scan configuration
    size_t recs_max = -1UL;		// maximum record count
    size_t order_min = 0;		// sequence No. range: min
    size_t order_max = -1UL;		// sequence No. range: max
    std::unordered_set<size_t> tids;	// thread id list
    std::unordered_set<size_t> pids;	// pid list
    c7::usec_t time_us_min = 0;		// time range: min
    c7::usec_t time_us_max = (-1UL)/2;	// time range: max
    uint32_t level_max = C7_LOG_BRF;	// C7_LOG_xxx
    uint32_t category_mask = 0;	// category selection mask

    std::string logname;

    // print configuration
    bool pr_category = false;		// print category
    bool pr_loglevel = false;		// print loglevel
    bool pr_pid = false;		// print PID
    std::string md_format;		// minidata format
    std::string tm_format;		// time format
    bool pr_threadname = false;		// print thread name
    bool pr_sourcename = false;		// print source name
    bool auto_tn_width = false;
    bool auto_sn_width = false;
    size_t tn_width = 3;		// max width of thread name
    size_t sn_width = 0;		// width of source name
    size_t sl_width = 3;		// width of source line

    // others
    bool clear = false;			// clear contents after print
};


/*----------------------------------------------------------------------------
                             args - print option
----------------------------------------------------------------------------*/

class print_args: public c7::args::parser {
public:
    print_args(mlog_conf& conf): conf_(conf) {}
    c7::result<> init() override;

private:
    mlog_conf& conf_;

    callback_t opt_category;
    callback_t opt_loglevel;
    callback_t opt_pid;
    callback_t opt_minidata;
    callback_t opt_thread_id;
    callback_t opt_thread_name;
    callback_t opt_source_name;
    callback_t opt_tm_format;
};

c7::result<>
print_args::init()
{
    c7::result<> res;
    {
	opt_desc d;
	d.short_name	= "c";
	d.opt_descrip	= "print category";
	res << add_opt(d, &print_args::opt_category);
    }
    {
	opt_desc d;
	d.short_name	= "g";
	d.opt_descrip	= "print log level";
	res << add_opt(d, &print_args::opt_loglevel);
    }
    {
	opt_desc d;
	d.short_name	= "p";
	d.opt_descrip	= "print pid";
	res << add_opt(d, &print_args::opt_pid);
    }
    {
	opt_desc d;
	d.long_name	= "mini";
	d.short_name	= "m";
	d.type		= opt_desc::prm_type::ANY;
	d.opt_descrip	= "print minidata";
	d.prm_descrip	= "format of c7::format [default: '(%{04x})']";
	d.prm_name	= "UINT64_FORMAT";
	d.prmc_min	= 0;
	d.prmc_max	= 1;
	res << add_opt(d, &print_args::opt_minidata);
    }
    {
	opt_desc d;
	d.long_name	= "thread-id";
	d.short_name	= "t";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print thread id";
	d.prm_descrip	= "width of thread id";
	d.prm_name	= "WIDTH";
	d.prmc_min	= 0;
	d.prmc_max	= 1;
	res << add_opt(d, &print_args::opt_thread_id);
    }
    {
	opt_desc d;
	d.long_name	= "thread";
	d.short_name	= "T";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print thread name";
	d.prm_descrip	= "width of thread name";
	d.prm_name	= "WIDTH";
	d.prmc_min	= 0;
	d.prmc_max	= 1;
	res << add_opt(d, &print_args::opt_thread_name);
    }
    {
	opt_desc d;
	d.long_name	= "source";
	d.short_name	= "s";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print source name and line number";
	d.prm_descrip	= "width of source name";
	d.prm_name	= "WIDTH";
	d.prmc_min	= 0;
	d.prmc_max	= 1;
	res << add_opt(d, &print_args::opt_source_name);
    }
    {
	opt_desc d;
	d.long_name	= "date";
	d.short_name	= "D";
	d.type		= opt_desc::prm_type::ANY;
	d.opt_descrip	= "date time format";
	d.prm_descrip	= "format of strftime()";
	d.prm_name	= "TIME_FORMAT";
	d.prmc_min	= 1;
	d.prmc_max	= 1;
	res << add_opt(d, &print_args::opt_tm_format);
    }
    return res;
};

c7::result<>
print_args::opt_category(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.pr_category = true;
    return c7result_ok();
}

c7::result<>
print_args::opt_loglevel(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.pr_loglevel = true;
    return c7result_ok();
}

c7::result<>
print_args::opt_pid(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.pr_pid = true;
    return c7result_ok();
}

c7::result<>
print_args::opt_minidata(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    if (vals.empty()) {
	conf_.md_format = "(%{04x})";
    } else {
	conf_.md_format = vals[0].param;
    }
    return c7result_ok();
}

c7::result<>
print_args::opt_thread_id(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    if (vals.empty()) {
	conf_.tn_width = 0;
    } else {
	conf_.tn_width = vals[0].u;
    }
    return c7result_ok();
}

c7::result<>
print_args::opt_thread_name(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.pr_threadname = true;
    if (vals.empty()) {
	conf_.tn_width = 0;
	conf_.auto_tn_width = true;
    } else {
	conf_.tn_width = vals[0].u;
    }
    return c7result_ok();
}

c7::result<>
print_args::opt_source_name(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.pr_sourcename = true;
    if (vals.empty()) {
	conf_.sn_width = 0;
	conf_.auto_sn_width = true;
    } else {
	conf_.sn_width = vals[0].u;
    }
    return c7result_ok();
}

c7::result<>
print_args::opt_tm_format(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.tm_format = vals[0].param;
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                              args - scan option
----------------------------------------------------------------------------*/

class scan_args: public c7::args::parser {
public:
    scan_args(mlog_conf& conf, std::function<void()> show_usage):
	conf_(conf), show_usage_(show_usage) {
    }

    c7::result<> init() override;

private:
    mlog_conf& conf_;
    std::function<void()> show_usage_;

    callback_t opt_max_line;
    callback_t opt_log_level;
    callback_t opt_category_list;
    callback_t opt_pid_list;
    callback_t opt_thread_list;
    callback_t opt_order_range;
    callback_t opt_date_range;
    callback_t opt_clear;
    callback_t opt_help;
};

c7::result<>
scan_args::init()
{
    c7::result<> res;
    {
	opt_desc d;
	d.long_name	= "record";
	d.short_name	= "r";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "maximum count of record";
	d.prm_descrip	= "count record";
	d.prm_name	= "MAX_COUNT";
	d.prmc_min	= 1;
	d.prmc_max	= 1;
	res << add_opt(d, &scan_args::opt_max_line);
    }
    {
	opt_desc d;
	d.long_name	= "level";
	d.short_name	= "g";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "maximum log level";
	d.prm_descrip	= "log level (0..7)";
	d.prm_name	= "LOG_LEVEL";
	d.prmc_min	= 1;
	d.prmc_max	= 1;
	res << add_opt(d, &scan_args::opt_log_level);
    }
    {
	opt_desc d;
	d.long_name	= "category";
	d.short_name	= "c";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print only specified category";
	d.prm_descrip	= "category (0..31)";
	d.prm_name	= "CATEGORY";
	d.prmc_min	= 1;
	d.prmc_max	= -1U;
	res << add_opt(d, &scan_args::opt_category_list);
    }
    {
	opt_desc d;
	d.long_name	= "pid";
	d.short_name	= "p";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print only specified pid";
	d.prm_descrip	= "pid";
	d.prm_name	= "PID";
	d.prmc_min	= 1;
	d.prmc_max	= -1U;
	res << add_opt(d, &scan_args::opt_pid_list);
    }
    {
	opt_desc d;
	d.long_name	= "thread";
	d.short_name	= "t";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "print only specified thread";
	d.prm_descrip	= "thread id";
	d.prm_name	= "THREAD_ID";
	d.prmc_min	= 1;
	d.prmc_max	= -1U;
	res << add_opt(d, &scan_args::opt_thread_list);
    }
    {
	opt_desc d;
	d.long_name	= "order";
	d.short_name	= "s";
	d.type		= opt_desc::prm_type::UINT;
	d.opt_descrip	= "range of order";
	d.prm_descrip	= "order of record";
	d.prm_name	= "ORDER";
	d.prmc_min	= 1;
	d.prmc_max	= 2;
	res << add_opt(d, &scan_args::opt_order_range);
    }
    {
	opt_desc d;
	d.long_name	= "date";
	d.short_name	= "d";
	d.type		= opt_desc::prm_type::DATE;
	d.opt_descrip	= "range of date time";
	d.prm_descrip	= "timestamp";
	d.prm_name	= "DATETIME";
	d.prmc_min	= 1;
	d.prmc_max	= 2;
	res << add_opt(d, &scan_args::opt_date_range);
    }
    {
	opt_desc d;
	d.long_name	= "clear";
	d.opt_descrip	= "clear contents after print";
	res << add_opt(d, &scan_args::opt_clear);
    }
    {
	opt_desc d;
	d.long_name	= "help";
	d.short_name	= "h";
	d.opt_descrip	= "this help";
	res << add_opt(d, &scan_args::opt_help);
    }
    return res;
}

c7::result<>
scan_args::opt_max_line(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.recs_max = vals[0].u;
    if (conf_.recs_max == 0) {
	conf_.recs_max = -1UL;
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_log_level(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    auto u = vals[0].u;
    if (u > C7_LOG_MAX) {
	return c7result_err(EINVAL, "%{}:%{} is too large.", desc.prm_name, u);
    }
    conf_.level_max = u;
    return c7result_ok();
}

c7::result<>
scan_args::opt_category_list(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    for (auto& v: vals) {
	if (v.u > C7_MLOG_C_MAX) {
	    return c7result_err(EINVAL, "%{}:%{} is too large.", desc.prm_name, v.u);
	}
	conf_.category_mask |= (1UL << v.u);
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_pid_list(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    for (auto& v: vals) {
	conf_.pids.insert(v.u);
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_thread_list(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    for (auto& v: vals) {
	conf_.tids.insert(v.u);
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_order_range(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.order_min = vals[0].u;
    if (vals.size() == 2) {
	conf_.order_max = vals[1].u;
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_date_range(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.time_us_min = vals[0].t * C7_TIME_S_us;
    if (vals.size() == 2) {
	conf_.time_us_max = vals[1].t * C7_TIME_S_us;
    } else {
	conf_.time_us_max = conf_.time_us_min + 60 * C7_TIME_S_us - 1;
    }
    return c7result_ok();
}

c7::result<>
scan_args::opt_clear(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    conf_.clear = true;
    return c7result_ok();
}

c7::result<>
scan_args::opt_help(const opt_desc& desc, const std::vector<opt_value>& vals)
{
    show_usage_();
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                args - parsing
----------------------------------------------------------------------------*/

static void show_usage(const c7::args::parser& scan,
		       const c7::args::parser& print)
{
    c7::strvec usage;
    usage.push_back(c7::format(
			"Usage: %{} [common option ...] LOG_NAME [print option ...]\n\n"
			" common option:\n",
			c7::app::progname));

    int main_indent = 2;
    int desc_indent = 32;
    scan.append_usage(usage, main_indent, desc_indent);
    usage.push_back(" print option:\n");
    print.append_usage(usage, main_indent, desc_indent);
    usage += "";
    
    auto s = usage
	| c7::nseq::transform([](auto& s){ return s+"\n"; })
	| c7::nseq::flat<1>()
	| c7::nseq::to_string();
    c7::drop = write(2, s.c_str(), s.size());
    std::exit(0);
}

static mlog_conf parse_args(char **argv)
{
    mlog_conf conf;
    print_args p_args{conf};
    scan_args s_args{conf, [&s_args, &p_args]() {
			       show_usage(s_args, p_args);
			   }};

    auto res = p_args.init();
    res << s_args.init();
    if (!res) {
	c7error(res);
    }

    // scan options ...
    if (auto res = s_args.parse(argv); !res) {
	c7error(res);
    } else {
	argv = res.value();
    }
    if (*argv == nullptr) {
	c7error("LOGNAME is not specified.");
    }
    
    conf.logname = *argv++;

    // print options ...
    if (auto res = p_args.parse(argv); !res) {
	c7error(res);
    } else {
	argv = res.value();
    }
    if (*argv != nullptr) {
	c7error(c7result_err(EINVAL, "too many parameter is specified: %{}.", *argv));
    }

    if (conf.tm_format.empty()) {
	conf.tm_format = "%m/%d %H:%M:%S";
    }
    conf.tm_format = "%{t" + conf.tm_format + "}";
    if (conf.category_mask == 0) {
	conf.category_mask = -1U;
    }

    return conf;
}


/*----------------------------------------------------------------------------
                                  mlog scan
----------------------------------------------------------------------------*/

static bool
choice(mlog_conf& conf, const c7::mlog_reader::info_t& info)
{
    if (!conf.tids.empty()) {
	if (conf.tids.find(info.thread_id) == conf.tids.end()) {
	    return false;
	}
    }
    if (!conf.pids.empty()) {
	if (conf.pids.find(info.pid) == conf.pids.end()) {
	    return false;
	}
    }
    if (info.time_us >= conf.time_us_min &&
	info.time_us <= conf.time_us_max &&
	info.weak_order >= conf.order_min &&
	info.weak_order <= conf.order_max &&
	info.level <= conf.level_max &&
	((1U << info.category) & conf.category_mask) != 0) {
	if (conf.auto_tn_width) {
	    conf.tn_width = std::max(conf.tn_width, info.thread_name.size());
	}
	if (conf.auto_sn_width) {
	    conf.sn_width = std::max(conf.sn_width, info.source_name.size());
	}
	if (info.source_line > 999) {
	    conf.sl_width = 4;
	}
	return true;
    }
    return false;
}

static bool
printlog(mlog_conf& conf, const c7::mlog_reader::info_t& info, void *data)
{
    // make prefix

    std::string prefix;
    c7::format(prefix, "%{4} ", info.weak_order);
    c7::format(prefix, conf.tm_format, info.time_us);

    if (conf.pr_threadname) {
	auto n = info.thread_name.size();
	n = (n <= conf.tn_width) ? 0 : (n - conf.tn_width);
	c7::format(prefix, " %{*}", conf.tn_width, &info.thread_name[n]);
    } else if (conf.tn_width > 0) {
	uint64_t tmax = (1UL << (conf.tn_width * 4));
	if (info.thread_id < tmax) {
	    c7::format(prefix, " @%{0*x}", conf.tn_width, info.thread_id);
	} else {
	    tmax >>= 4;
	    tmax--;
	    c7::format(prefix, " @*%{0*x}", conf.tn_width-1, (tmax & info.thread_id));
	}
    }

    if (conf.pr_pid) {
	c7::format(prefix, "/%{06}", info.pid);
    }

    if (conf.sn_width > 0) {
	if (info.source_name[0] != 0) {
	    auto n = info.source_name.size();
	    n = (n <= conf.sn_width) ? 0 : (n - conf.sn_width);
	    c7::format(prefix, " %{*}:%{*}",
		       conf.sn_width, &info.source_name[n],
		       conf.sl_width, info.source_line);
	} else {
	    c7::format(prefix, " %{*}:%{*}",
		       conf.sn_width, "---",
		       conf.sl_width, "---");
	}
    }

    if (conf.pr_category && conf.pr_loglevel) {
	c7::format(prefix, " [%{2}:%{1}]", info.category, info.level);
    } else if (conf.pr_category) {
	c7::format(prefix, " [%{2}]", info.category);
    } else if (conf.pr_loglevel) {
	c7::format(prefix, " [%{1}]", info.level);
    } else {
	prefix += ' ';
    }

    if (!conf.md_format.empty()) {
	c7::format(prefix, conf.md_format, info.minidata);
    }

    prefix += ": ";

    // print data with prefix

    char *p, *s = static_cast<char *>(data);
    while ((p = std::strchr(s, '\n')) != NULL) {
	std::cout << prefix << std::string_view{s, (p - s) + 1UL};
	s = p + 1;
    }
    if (*s != 0) {
	std::cout << prefix << s << std::endl;
    }

    return true;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    mlog_conf conf = parse_args(++argv);

    if (auto res = conf.reader.load(conf.logname); !res) {
	c7error(res);
    }

    conf.reader.scan(conf.recs_max,
		     conf.order_min,
		     conf.time_us_min,
		     [&conf](auto info){ return choice(conf, info); },
		     [&conf](auto info, auto data){ return printlog(conf, info, data); });

    if (conf.clear) {
	c7::mlog_clear(conf.logname);
    }

    return 0;
}
