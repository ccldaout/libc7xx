#include <c7format.hpp>
#include <c7typefunc.hpp>

#define p_	c7::p_

template <typename... Args>
static void test_is_empty(const std::string& s, Args... args)
{
    p_("%{} :  %{}", s, c7::typefunc::is_empty_v<Args...>);
}

int main()
{
    test_is_empty("1", 1);
    test_is_empty("1 2", 1, 2);
    test_is_empty("empty");

    return 0;
}
