#include <c7format.hpp>
#include <c7coroutine.hpp>
#include <cstring>


#define p_	c7::p_


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

struct data_t {
    int index;
    std::string s;
};

static void makedata()
{
    for (int i = 0; i < 5; i++) {
	c7::generator<data_t>::yield(data_t{ i, c7::format("data%{}", i) });
    }
}

static void makedata2()
{
    std::vector<data_t> vec;
    for (int i = 0; i < 5; i++) {
	vec.push_back(data_t{ i, c7::format("data%{}", i) });
    }
    for (auto& v: vec) {
	c7::generator<data_t>::yield(v);
    }
}

static void makedata3()
{
    std::vector<data_t> vec;
    for (int i = 0; i < 5; i++) {
	vec.push_back(data_t{ i, c7::format("data%{}", i) });
    }
    for (auto& v: vec) {
	c7::generator<data_t&>::yield(v);
    }
    p_(" - - - ");
    for (auto& v: vec) {
	p_("v:%{}, index:%{}, s:%{}", &v, v.index, v.s);
    }
}

static void test_generator()
{
    p_("------------------------------");
    p_("test_generator start");

    {
	c7::generator<std::string> gen(
	    1024,
	    []() {
		using gen = c7::generator<std::string>;
		gen::yield("These");
		gen::yield("data");
		gen::yield("are");
		gen::yield("from");
		gen::yield("generator.");
	    });

	for (auto& s: gen) {
	    p_("<%{}>", s);
	}
	p_("is_success: %{}", gen.is_success());
    }

    p_("----");
    {
	int i = 0;
	c7::generator<std::string> gen(
	    1024,
	    []() {
		using gen = c7::generator<std::string>;
		gen::yield("These");
		gen::yield("data");
		gen::yield("are");
		gen::yield("from");
		gen::yield("generator.");
	    });

	for (auto& s: gen) {
	    p_("<%{}>", s);
	    if (i++ == 2)
		break;
	}
	p_("is_success: %{}", gen.is_success());
    }

    p_("---- int literal");
    {
	c7::generator<int> gen(
	    1024,
	    []() {
		using gen = c7::generator<int>;
		gen::yield(1);
		gen::yield(2);
		gen::yield(3);
	    });

	for (auto& n: gen) {
	    p_("<%{}, %{}>", &n, n);
	}
    }

    p_("---- vec<int>");
    {
	c7::generator<int> gen(
	    1024,
	    []() {
		std::vector vec{1,2,3};
		using gen = c7::generator<int>;
		for (auto& v: vec) {
		    gen::yield(v);
		}
	    });

	for (auto& n: gen) {
	    p_("<%{}, %{}>", &n, n);
	}
    }

    p_("---- vec<int> & gen<int&>");
    {
	c7::generator<int&> gen(
	    1024,
	    []() {
		std::vector vec{1,2,3};
		using gen = c7::generator<int&>;
		for (auto& v: vec) {
		    gen::yield(v);
		}
		for (auto& v: vec) {
		    p_("vec: %{} %{}", &v, v);
		}
	    });

	for (auto& n: gen) {
	    p_("<%{}, %{}>", &n, n);
	    n *= 2;
	}
    }

    p_("---- makedata ");
    {
	for (auto& idx: c7::generator<data_t>(1024, makedata)) {
	    p_("idx:%{}, index:%{}, s:%{}", &idx, idx.index, idx.s);
	}
    }

    p_("---- makedata2 ");
    {
	for (auto& idx: c7::generator<data_t>(1024, makedata2)) {
	    p_("idx:%{}, index:%{}, s:%{}", &idx, idx.index, idx.s);
	    //idx.index *= 2;	// ERROR
	}
    }

    p_("---- makedata3");
    {
	for (auto& idx: c7::generator<data_t&>(1024, makedata3)) {
	    p_("idx:%{}, index:%{}, s:%{}", &idx, idx.index, idx.s);
	    idx.index *= 2;
	}
    }

    p_("test_generator end");
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

int main()
{
    test_generator();
    return 0;
}
