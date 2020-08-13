#include "c7format.hpp"
#include "c7result.hpp"

using namespace std;
using namespace c7;
#define p_ c7::p_

static result<std::string> func3()
{
    static int n;
    if (n++ % 2 == 0)
	return result<std::string>("good morning");
    else
	return c7result_err(333, "func3");
}

static result<std::string> func2()
{
    auto r = func3();
    p_("func2: result: %{}", r);
    if (r) {
	p_("func2: value:%{}", r.value());
	return r;
    }
    return c7result_err(std::move(r), "func%{}", 2);
}

static result<void> func1()
{
    auto r = func2();
    p_("func1: result: %{}", r);
    if (r) 
	return result<void>(std::move(r));
    else
	return c7result_err(std::move(r), 111, "func1");
}

int main()
{
    for (int i = 0; i < 2; i++) {
	p_("--- #%{} ---", i);
	auto r = func1();
	if (!r) {
	    p_("main: has error");
	}
	p_("main: result: %{}", r);
    }
    return 0;
}
