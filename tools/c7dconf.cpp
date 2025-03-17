/*
 * c7dconf.cpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7app.hpp>
#include <c7dconf.hpp>
#include <c7string.hpp>


using c7::p_;
using c7::P_;


static void showusage()
{
    p_("Usage: %{} NAME show\n"
       "       %{} NAME get {IDENT|INDEX}\n"
       "       %{} NAME set {IDENT|INDEX} VALUE [...]",
       c7::app::progname,
       c7::app::progname,
       c7::app::progname);
    std::exit(0);
}


static void showdconf(const std::vector<c7::dconf_def>& defs)
{
    size_t id_maxlen = 0;
    for (auto& d: defs) {
	id_maxlen = std::max(id_maxlen, d.ident.size());
    }
    for (auto& d: defs) {
	if (d.type == C7_DCONF_TYPE_I64) {
	    p_("[%{3}] %{<*} : %{15} ... %{}",
	       d.index, id_maxlen, d.ident, c7::dconf[d.index].i, d.descrip);
	} else {
	    p_("[%{3}] %{<*} : %{15} ... %{}",
	       d.index, id_maxlen, d.ident, c7::dconf[d.index].r, d.descrip);
	}
    }
    std::exit(0);
}


static const c7::dconf_def *getdef(const std::vector<c7::dconf_def>& defs, char *s)
{
    char *p;
    int index = std::strtol(s, &p, 0);
    if (*p == 0) {
	for (auto& d: defs) {
	    if (d.index == index) {
		return &d;
	    }
	}
    } else {
	for (auto& d: defs) {
	    if (d.ident == s) {
		return &d;
	    }
	}
    }
    return nullptr;
}

static void getvalue(const std::vector<c7::dconf_def>& defs, char *s)
{
    auto def = getdef(defs, s);
    if (def == nullptr) {
	c7error("invalid IDENT or INDEX: '%{}'", s);
    }
    if (def->type == C7_DCONF_TYPE_I64) {
	P_("%{}", c7::dconf[def->index].i);
    } else {
	P_("%{g}", c7::dconf[def->index].r);
    }
    std::exit(0);
}


static void setvalues(const std::vector<c7::dconf_def>& defs, char **argv)
{
    while (*argv) {
	auto def = getdef(defs, *argv);
	if (def == nullptr) {
	    c7error("invalid IDENT or INDEX: '%{}'", *argv);
	}
	if (*++argv == nullptr){
	    c7error("VALUE is required for IDENT(INDEX) '%{}'", *--argv);
	}
	if (def->type == C7_DCONF_TYPE_I64) {
	    char *p;
	    int64_t v = std::strtol(*argv, &p, 0);
	    if (*p != 0) {
		c7exit("integer is required for IDENT(INDEX) '%{}'", *argv);
	    }
	    c7::dconf[def->index].i = v;
	    if (def->index == C7_DCONF_MLOG_obsolete) {
		c7::dconf[C7_DCONF_MLOG].i = v;
	    } else if (def->index == C7_DCONF_MLOG) {
		c7::dconf[C7_DCONF_MLOG_obsolete].i = v;
	    }
	} else {
	    char *p;
	    double v = std::strtod(*argv, &p);
	    if (*p != 0) {
		c7exit("real is required for IDENT(INDEX) '%{}'", *argv);
	    }
	    c7::dconf[def->index].r = v;
	}
	argv++;
    }
    std::exit(0);
}


int main(int argc, char **argv)
{
    if (argc <= 2 || c7::strmatch(argv[1], "-h", "--help") != -1) {
	showusage();
    }
    argv++;

    std::vector<c7::dconf_def> defs;
    if (auto res = c7::dconf.load(*argv); !res) {
	c7error(res);
    } else {
	defs = std::move(res.value());
    }

    argv++;
    if (std::strcmp(*argv, "show") == 0) {
	if (argv[1] != nullptr) {
	    c7error("too many parameter(s): %{}", argv[1]);
	}
	showdconf(defs);
    } else if (std::strcmp(*argv, "get") == 0) {
	if (argv[1] == nullptr || argv[2] != nullptr) {
	    c7error("'get' command require just one parameter");
	}
	getvalue(defs, argv[1]);
    } else if (std::strcmp(*argv, "set") == 0) {
	setvalues(defs, argv+1);
    }

    return 0;
}
