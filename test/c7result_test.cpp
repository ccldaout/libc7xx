#include <c7format.hpp>
#include <c7result.hpp>

using namespace std;
using namespace c7;
#define p_ c7::p_

result<std::string> func3()
{
    static int n;
    if (n++ % 2 == 0)
	return result<std::string>("good morning");
    else
	return c7result_err(333, "func3");
}

result<std::string> func2()
{
    auto r = func3();
    p_("func2: result: %{}", r);
    if (r) {
	p_("func2: value:%{}", r.value());
	return r;
    }
    return c7result_err(std::move(r), "func%{}", 2);
}

result<> func1()
{
    auto r = func2();
    p_("func1: result: %{}", r);
    if (r) 
	return result<>(std::move(r));
    else
	return c7result_err(std::move(r), 111, "func1");
}

result<> func0()
{
    auto r = func1();
    p_("func0: result: %{}", r);
    if (r) 
	return result<>(std::move(r));
    else
	return c7result_err(std::move(r), ENOSPC);
}

result<> funcM()
{
    result<std::string> res0 = c7result_err(EINVAL, "1st error");
    result<std::string> res1 = c7result_err(ENOMEM, "end error");
    p_("res0: %{}", res0);
    p_("res1: %{}", res1);
    p_("--");
    res0.merge_iferror(std::move(res1));
    p_("res0: %{}", res0);
    p_("res1: %{}", res1);

    p_("--");
    auto res3 = c7result_ok();
    auto res4 = c7result_ok();
    res3.merge_iferror(std::move(res4));
    p_("res3: %{}", res3);

    p_("--");
    result<> res5 = c7result_err(EPERM);
    auto res6 = c7result_ok();
    res5.merge_iferror(std::move(res6));
    p_("res5: %{}", res5);

    p_("--");
    auto res7 = c7result_ok();
    auto res8 = c7result_err(EINTR);
    res7.merge_iferror(std::move(res8));
    p_("res7: %{}", res7);

    p_("-- return --");
    return res7;
}

int main()
{
    for (int i = 0; i < 2; i++) {
	p_("--- #%{} ---", i);
	try {
	    auto r = func0();
	    if (!r) {
		p_("main: has error");
		//r.clear();
	    }
	    p_("main: result: %{}", r);
	} catch (std::exception& e) {
	    p_("exception: %{}", e.what());
	}
    }

    p_("--------------");
    {
	result<std::string> res = c7result_err(EINVAL, "error");
	p_("res.value: '%{}'", res.value("this is default"));

	res = c7result_ok<std::string>("success");
	p_("res.value: '%{}'", res.value("this is default"));
    }

    p_("--------------");
    {
	result<std::string> res0 = c7result_err(EINVAL, "error");
	result<std::string> res1 = c7result_err(std::move(res0), "add error");
	result<> res2;
	res1.copy(res2);
	p_("res0: %{}", res0);
	p_("res1: %{}", res1);
	p_("res2: %{}", res2);
    }

    p_("--------------");
    {
	p_("funcM: %{}", funcM());
    }

    return 0;
}
