#include <c7format.hpp>
#include <c7string.hpp>

using namespace std;
#define f_ c7::format
#define p_ c7::p_


static void test_match()
{
    p_("-- match -----------------------");

    {
	p_("head: 2:%{}",
	   c7::strmatch_head("defgh", "abc", "defghi", "def", "gh"));
	p_("head: -1:%{}",
	   c7::strmatch_head("xyz", "abc", "defghi", "def", "gh"));

	const char *candv[] = {"abc", "defghi", "def", "gh", nullptr};
	p_("head: 1:%{}",
	   c7::strmatch_head("defghij", candv));

	std::vector<std::string> cands;
	for (auto c = candv; *c != nullptr; c++)
	    cands.push_back(std::string(*c));

	p_("head: 3:%{}",
	   c7::strmatch_head("ghijk", cands));
	p_("head: -1:%{}",
	   c7::strmatch_head("xyz", cands));
    }


    {
	p_("tail: 2:%{}",
	   c7::strmatch_tail("abc.c", ".cpp", ".hpp", ".c", ".h"));
	p_("tail: -1:%{}",
	   c7::strmatch_tail("abc.C", ".cpp", ".hpp", ".c", ".h"));

	const char *candv[] = {".cpp", ".hpp", ".c", ".h", nullptr};
	p_("tail: 1:%{}",
	   c7::strmatch_tail("abc.hpp", candv));

	std::vector<std::string> cands;
	for (auto c = candv; *c != nullptr; c++)
	    cands.push_back(std::string(*c));

	p_("tail: 3:%{}",
	   c7::strmatch_tail("abc.h", cands));
	p_("tail: -1:%{}",
	   c7::strmatch_tail("abc.C", cands));
    }
}


static void test_uplow()
{
    p_("-- uplow -----------------------");
    
    std::string s1("upper:");
    p_("<%{}>", c7::str::upper(s1, "Hello World"));

    std::stringstream s2;
    s2 << "lower:";
    p_("<%{}>", c7::str::lower(s2, "Hello World", 7));
}


static void test_concat()
{
    p_("-- concat -----------------------");

    const char *sv[] = {"one", "two", "three", "four", nullptr};
    std::string s1;
    p_("concat <%{}>", c7::str::concat(s1, ", ", sv));

    std::vector<std::string> sv2;
    for (auto p = sv; *p != nullptr; p++)
	sv2.push_back(*p);
    std::stringstream ss;
    p_("concat <%{}>", c7::str::concat(ss, "++", sv2));
}


static void test_trim()
{
    p_("-- trim -----------------------");
    const char *in = " \t\t message \t\n";
    p_("in:<%{}>", in);
    p_("trim:<%{}>", c7::str::trim(in));
    p_("trim_left:<%{}>", c7::str::trim_left(in));
    p_("trim_right:<%{}>", c7::str::trim_right(in));
}


static void test_transpose()
{
    p_("-- transpose -----------------------");
    const char *in = "miwanaga@jeol.co.jp";
    p_("<%{}>\n<%{}>", in, c7::str::transpose(in, "aiueo@mwngjl", "AIUEO.*"));
    p_("<%{}>\n<%{}>", in, c7::str::transpose(in, [](char ch){ return ch+1; }));
}


static void test_split()
{
    p_("-- split -----------------------");

    const char *in = "  my\t: name\t\t:is \t : iwanaga ";

    for (auto s: c7::str::split_trim(in, ':'))
	p_("<%{}>", s);

    p_("-- split + lambda -> vector");

    for (auto s: c7::str::split(in, ':', [](const std::string& s){
		return c7::str::transpose(c7::str::trim(s),
					  [](char ch){ return std::toupper(ch); });
	    }))
	p_("<%{}>", s);

    p_("-- split_for (const std:string&)");

    c7::str::split_for(in, ':', [](const std::string& s){
	    p_("<%{}>", s);
	});

    p_("-- split_for (const char *)");

    c7::str::split_for(in, ':', [](const char *s, const char *e){
	    p_("<%{}>", std::string(s, e - s));
	});

    p_("-- split_gen");

    for (auto s: c7::str::split_gen(in, ':')) {
	    p_("<%{}>", s);
    }
}


static void test_Cescape()
{
    p_("-- Cescape -----------------------");
    std::string s("string: ");
    std::stringstream ss;
    ss << "stream: ";

    c7::str::eval_C(s, "tab<\\t>\\x1b[33mshow\\033[0mdown");
    c7::str::eval_C(ss, "tab<\\t>\\x1b[33mshow\\033[0mdown");

    p_("%{}", s);
    p_("%{}", ss.str());
}


static void test_eval()
{
    p_("-- eval -----------------------");

    {
	std::string s;
	auto r = c7::str::eval_env(s, "HOME=${HOME}");
	if (r)
	    p_("ok:<%{}>", s);
	else
	    p_("ng:%{}", r);
    }
    p_("---");
    {
	std::string s;
	auto r = c7::str::eval_env(s, "APPPATH=${APPPATH:-${HOME}/app}${OOPS:-oops}");
	if (r)
	    p_("ok:<%{}>", s);
	else
	    p_("ng:%{}", r);
    }
    {
	std::string s;
	auto r = c7::str::eval_env(s, "APPPATH=${APPPATH:-${HOME}/app${OOPS:-oops}");
	if (r)
	    p_("ok:<%{}>", s);
	else
	    p_("ng:%{}", r);
    }
    p_("---");
    {
	auto r = c7::str::eval_env("APPPATH=${APPPATH:-${HOME}/app}${OOPS:-oops}");
	if (r)
	    p_("ok:<%{}>", r.value());
	else
	    p_("ng:%{}", r);
    }
    {
	auto r = c7::str::eval_env("APPPATH=${APPPATH:-${HOME}/app${OOPS:-oops}");
	if (r)
	    p_("ok:<%{}>", r.value());
	else
	    p_("ng:%{}", r);
    }
}


int main()
{
    test_match();
    test_uplow();
    test_concat();
    test_trim();
    test_transpose();
    test_split();
    test_Cescape();
    test_eval();

    return 0;
}
