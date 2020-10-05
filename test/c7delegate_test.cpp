#include <c7delegate.hpp>
#include <c7format.hpp>

static bool func1_false(const int& v)
{
    c7::p_("func1_false: %{}", v);
    return false;
}

static bool func2_false(const int& v)
{
    c7::p_("func2_false: %{}", v);
    return false;
}

static bool func3_true(const int& v)
{
    c7::p_("func3_true  : %{}", v);
    return true;
}

static bool func4_false(const int& v)
{
    c7::p_("func4_false: %{}", v);
    return false;
}

static bool func5_true(const int& v)
{
    c7::p_("func5_true  : %{}", v);
    return true;
}

int main()
{
    {
	c7::delegate<bool, const int&> op;

	op.push_back(func1_false);
	op += func2_false;
	auto id3 = op.push_back(func3_true);
	op.push_back(func4_false);
	auto id5 = op.push_back(func5_true);

	c7::p_("----- status: %{}", op(1));

	op.remove(id3);
	c7::p_("func3_true ... removed");
	c7::p_("----- status: %{}", op(2));

	op.remove(id5);
	c7::p_("func5_true ... removed");
	c7::p_("----- status: %{}", op(3));
    }    

    return 0;
}
