#include "c7format.hpp"
#include "c7path.hpp"


using namespace std;

#define p_	c7::p_


static void untildize_test()
{
    p_("--- untildize --------------------------------");

    std::vector<std::string> pv{"~/ebsys", "~", "~jeoleb", "~jeoleb/job", "~whoami/aa"};
    for (auto& path: pv) {
	p_("%{} -> %{}", path, c7::path::untildize(path));
    }
}


static void cwd_test()
{
    p_("--- cwd --------------------------------");

    auto res = c7::path::cwd();
    if (res)
	p_("cwd -> <%{}>", res.value());
    else
	p_("cwd failed: %{}", res);
}

static void ortho_test()
{
    p_("--- ortho --------------------------------");

    std::vector<std::string> pv = {
	"../a/aa/../b/c/d",
	"/a/b/../../a/aa////bb/../../b/c",
	"a/b/../../a/aa/bb/../../b/c/..",
    };
    for (auto& path: pv) {
	p_("%{} -> %{}", path, c7::path::ortho(path));
    }
}


static void abs_test()
{
    p_("--- abs --------------------------------");

    {
	const char *path = "../a/bb/../b/c";
	if (auto res = c7::path::abs(path); res)
	    p_("%{} -> %{}", path, res.value());
	else
	    p_("%{} -> %{}", path, res);
    }
    {
	const char *path = "a/b/c";
	const char *base = "~jeoleb";
	if (auto res = c7::path::abs(path, base); res)
	    p_("%{} %{} -> %{}", path, base, res.value());
	else
	    p_("%{} %{} -> %{}", path, base, res);
    }
    {
	const char *path = "~ebsys/../a/b/c";
	const char *base = "~noname";
	if (auto res = c7::path::abs(path, base); res)
	    p_("%{} %{} -> %{}", path, base, res.value());
	else
	    p_("%{} %{} -> %{}", path, base, res);
    }
    {
	const char *path = "a/b/c";
	const char *base = "~noname";
	if (auto res = c7::path::abs(path, std::string(base)); res)
	    p_("%{} %{} -> %{}", path, base, res.value());
	else
	    p_("%{} %{} -> %{}", path, base, res);
    }
}


static void search_test()
{
    p_("--- search --------------------------------");

    p_("-- vector<string> --");

    std::vector<std::string> pathlist = {
	"/usr/ebsys:/usr/ebsys/tmp",
	"/usr/bin:/usr/sbin",
	"/usr/ebsys/.ccldaout/libc7/src:/usr/ebsys/prog/c7xxdev:/usr/ebsys/uukit/prog/yamm",
    };
    {
	auto res = c7::path::search("yamm.h", pathlist, ".c");
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }

    {
	auto res = c7::path::search("c7thread", pathlist, ".hpp");
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }

    {
	auto res = c7::path::search("notfound", pathlist);
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }

    p_("-- char*[] --");

    const char *pathlistv[] = {
	"/usr/ebsys:/usr/ebsys/tmp",
	"/usr/bin:/usr/sbin",
	"/usr/ebsys/.ccldaout/libc7/src:/usr/ebsys/prog/c7xxdev:/usr/ebsys/uukit/prog/yamm",
    };
    {
	auto res = c7::path::search("yamm.h", pathlistv, ".c");
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }

    {
	auto res = c7::path::search("c7thread", pathlistv, ".hpp");
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }

    {
	auto res = c7::path::search("notfound", pathlistv);
	if (res)
	    p_("find %{}", res.value());
	else
	    p_("error %{}", res);
    }
}


int main()
{
    untildize_test();
    cwd_test();
    ortho_test();
    abs_test();
    search_test();

    return 0;
}
