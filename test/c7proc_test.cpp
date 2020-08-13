#include "c7format.hpp"
#include "c7proc.hpp"

#define p_	c7::p_

static void proc1_test()
{
    p_("- proc1 sleep 1 -----------------------------------------------");

    auto proc = c7::proc();

    proc.on_start += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_start: %{}", p);
	});
    proc.on_finish += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_finish: %{}", p);
	});

    p_("start ...");
    auto r = proc.start("sleep", std::vector<std::string>({"sleep", "1"}));
    p_("start ... %{}", r);
    p_("wait...");
    auto res = proc.wait();
    p_("wait... %{}", res);
    if (res) {
	auto [state, value] = proc.state();
	p_("ok: %{} %{}", state, value);
    }
}

static void proc1a_test()
{
    p_("- proc1 sleep 0 -----------------------------------------------");

    auto proc = c7::proc();

    proc.on_start += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_start: %{}", p);
	});
    proc.on_finish += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_finish: %{}", p);
	});

    p_("start ...");
    auto r = proc.start("sleep", std::vector<std::string>({"sleep", "0"}));
    p_("start ... %{}", r);
    p_("wait...");
    auto res = proc.wait();
    p_("wait... %{}", res);
    if (res) {
	auto [state, value] = proc.state();
	p_("ok: %{} %{}", state, value);
    }
}

static void proc2_test()
{
    p_("- proc2 not found -----------------------------------------------");

    auto proc = c7::proc();

    proc.on_start += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_start: %{}", p);
	});
    proc.on_finish += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_finish: %{}", p);
	});

    p_("start ...");
    auto r = proc.start("not found", std::vector<std::string>({"-", "1"}));
    p_("start ... %{}", r);
    p_("wait...");
    auto res = proc.wait();
    p_("wait... %{}", res);
    if (res) {
	auto [state, value] = proc.state();
	p_("ok: %{} %{}", state, value);
    }
}

static void proc3_test()
{
    p_("- proc preexec false -----------------------------------------------");

    auto proc = c7::proc();

    proc.on_start += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_start: %{}", p);
	});
    proc.on_finish += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_finish: %{}", p);
	});

    p_("start ...");
    auto r = proc.start("sleep", std::vector<std::string>({"sleep", "1"}),
			[](){ p_("preexec"); return false; });
    p_("start ... %{}", r);
    p_("wait...");
    auto res = proc.wait();
    p_("wait... %{}", res);
    if (res) {
	auto [state, value] = proc.state();
	p_("ok: %{} %{}", state, value);
    }
}

static void proc4_test()
{
    p_("- proc4 crash -----------------------------------------------");

    auto proc = c7::proc();

    proc.on_start += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_start: %{}", p);
	});
    proc.on_finish += std::function<void(c7::proc::proxy)>(
	[](auto p) {
	    p_("on_finish: %{}", p);
	});

    p_("start ...");
    auto r = proc.start("abort.exe", std::vector<std::string>({"abort.exe"}));
    p_("start ... %{}", r);
    p_("wait...");
    auto res = proc.wait();
    p_("wait... %{}", res);
    if (res) {
	auto [state, value] = proc.state();
	p_("ok: %{} %{}", state, value);
    }
}

int main()
{
    proc1_test();
    proc1a_test();
    proc2_test();
    proc3_test();
    proc4_test();

    return 0;
}
