#include "c7any.hpp"
#include "c7format.hpp"
#include <memory>
#include <iostream>


using namespace std;
using namespace c7;
#define p_	c7::p_


/*----------------------------------------------------------------------------
                                   anydata
----------------------------------------------------------------------------*/

struct mixdata {
    int a, b;
    mixdata(int a, int b): a(a), b(b) {}
    mixdata(mixdata&& m): a(m.a), b(m.b) {
	cout << "mixdata moved" << endl;
	m.a = m.b = 0;
    }
    mixdata(const mixdata& m): a(m.a), b(m.b) {}
    mixdata& operator=(const mixdata& m) {
	if (this != &m)
	    *this = m;
	return *this;
    }
};

struct mixdata2 {
    char data[256];
    int a, b, c;
};

static void anydata_test()
{
    {
	c7::anydata cap;
	std::string s("Hello");
	cap = c7::anydata(std::move(s));
	auto s2 = cap.move<std::string>();
	cout << "s:" << s << ", s2:" << s2 << endl;
	s2 = cap.move<std::string>();
	cout << "s:" << s << ", s2:" << s2 << endl;
	s2 = cap.move<std::string>();
	cout << "s:" << s << ", s2:" << s2 << endl;

	s = "World";
	cap = c7::anydata(std::move(s));
	auto s3 = cap.move<std::string>();
	cout << "s:" << s << ", s3:" << s3 << endl;
    }

    p_("--------------------------------------");

    {
	mixdata m0(1, 2);
	cout << "----" << endl;
	c7::anydata b0(m0);
	cout << "-" << endl;
	auto d0 = b0.move<mixdata>();
	cout << "-" << endl;
	cout << d0.a << ", " << d0.b << endl;

	cout << "----" << endl;
	c7::anydata b1(&m0);
	cout << "-" << endl;
	auto d1 = b1.move<mixdata*>();
	cout << "-" << endl;
	cout << d1->a << ", " << d1->b << endl;

	cout << "----" << endl;
	c7::anydata b2(std::move(m0));
	cout << "-" << endl;
	auto d2 = b2.move<mixdata>();
	cout << "-" << endl;
	cout << d2.a << ", " << d2.b << endl;
	cout << m0.a << ", " << m0.b << endl;

	cout << "----" << endl;
	b0 = std::move(b2);
	cout << "-" << endl;
	auto d3 = b0.move<mixdata>();
	cout << "-" << endl;
	cout << d3.a << ", " << d3.b << endl;
	cout << "-!-" << endl;
	//d2 = b2.move<mixdata>();	// crash
	//cout << "-" << endl;
	//cout << d2.a << ", " << d2.b << endl;
    }

    p_("--------------------------------------");
}


/*----------------------------------------------------------------------------
                                   anyfunc
----------------------------------------------------------------------------*/

void anyfunc_test()
{
    p_("--------------------------------------");

    auto g = [](int n, double d) { p_("g: n:%{}, d:%{}", n, d); };
    auto g2 = [](int n, double d) { p_("g2: n:%{}, d:%{}", n, d); return true; };

    //anyfunc df(lambda<void, int, double>(g));		// <- parsed as function declaration
    anyfunc df1;
    df1 = decltype(df1)::args<int, double>(g);
    df1(1, 3.14);

    anyfunc<bool> df2;
    df2 = decltype(df2)::args<int, double>(g2);
    auto b = df2(2, 1.414);
    p_("g2 -> %{}", b);

    auto g3 = [](int age, const std::string& name) { p_("age:%{}, name:%{}", age, name); };
    df1 = decltype(df1)::args<int, const std::string&>(g3);
    g3(53, "papa");

    df1 = [](){ p_("no argument"); };
    df1.call();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

int main()
{
    anydata_test();
    anyfunc_test();
    return 0;
}
