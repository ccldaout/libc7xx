#include <c7format.hpp>
#include <c7proc.hpp>
#include <c7utils.hpp>
#include <csignal>

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
    c7::sleep_ms(500);
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
    auto r = proc.start("not_found", std::vector<std::string>({"-", "1"}));
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
    p_("- proc3 preexec false -----------------------------------------------");

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
    auto r = proc.start("sleep", std::vector<std::string>({"-preexe_failed-", "1"}),
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
    p_("- proc4 killed -----------------------------------------------");

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
    auto r = proc.start("sleep", std::vector<std::string>({"-killed-", "10"}));
    p_("start ... %{}", r);
    proc.signal(SIGBUS);
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
