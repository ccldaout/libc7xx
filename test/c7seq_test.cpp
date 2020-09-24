#include <c7format.hpp>
#include <c7seq.hpp>
#include <iostream>
#include <vector>
#include <typeinfo>

using namespace std;
namespace seq = c7::seq;

#define p_	c7::p_


template <typename Seq>
static void proxy1(Seq&& s)
{
    for (auto v: s) {
	p_("%{}", v);
    }
}

template <typename Seq>
static void proxy2(Seq s)
{
    for (auto v: s) {
	p_("%{}", v);
    }
}

static void range_test()
{
    p_("-- range ----------------------------------------------");
    {
	for (auto v: seq::range<int>(5)) {
	    p_("%{}", v);
	}
	p_("-");
	for (auto v: seq::range<double>(5, 0.01, 0.10)) {
	    p_("%{}", v);
	}
    }
}

static void combo1_test()
{
    p_("-- head,tail,reverse,filter combinatione ------------#1");
    {
	std::vector<int> vec{ 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 };

	p_("--- head(vec) ---");
	for (auto v: seq::head(vec, 4)) {
	    p_("%{}", v);
	}

	p_("--- head(range()) ---");
	for (auto v: seq::head(seq::range<int>(10, 100, 10), 4)) {
	    p_("%{}", v);
	}

	p_("--- tail(range()) ---");
	for (auto v: seq::tail(seq::range<int>(10, 100, 10), 4)) {
	    p_("%{}", v);
	}

	p_("--- reverse(vec) ---");
	for (auto v: seq::reverse(vec)) {
	    p_("%{}", v);
	}

	p_("--- filter(reverse(vec)) ---");
	for (auto v: seq::filter(seq::reverse(vec), [](const int& u){ return (u%3 == 0);})) {
	    p_("%{}", v);
	}
    }
}

static void combo2_test()
{
    p_("-- head,tail,reverse,filter combinatione ------------#2");
    {
	std::vector<int> vec{ 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 };
	for (auto& v: vec) {
	    p_("%{} : %{}", v, &v);
	}

	p_("--- head(vec) ---");
	for (auto& v: seq::head(vec, 4)) {
	    p_("%{} : %{}", v, &v);
	}

	p_("--- head(head(vec)) ---");
	for (auto& v: seq::head(seq::head(vec, 7), 3)) {
	    p_("%{} : %{}", v, &v);
	}
	
	p_("--- proxy1(head(vec)) ---");
	proxy1(seq::head(vec, 3));
	
//	p_("--- proxy1(move(head(vec))) ---");
//	proxy1(std::move(seq::head(vec, 3)));
	
	p_("--- proxy2(head(vec)) ---");
	proxy2(seq::head(vec, 3));

//	p_("--- proxy2(move(head(vec))) ---");
//	proxy2(std::move(seq::head(vec, 3)));

	p_("--- proxy1(h1=head(vec)) ---");
	auto h1 = seq::head(vec, 3);
	proxy1(h1);

	p_("--- proxy2(h2=head(vec)) ---");
	auto h2 = seq::head(vec, 3);
	proxy2(h2);
    }
}

static void enum_test()
{
    p_("-- enumerate ------------------------------------------");
    {
	std::vector<double> vec;
	for (auto v: seq::range(100, 1.0, 0.01))
	    vec.push_back(v);
	for (auto& v: seq::head(seq::tail(vec, 7), 5)) {
	    p_("%{} %{}", v, &v);
	}

	p_("-");

	for (auto v: seq::enumerate(seq::head(seq::tail(vec, 7), 5), 7)) {
	    auto& d = v.second;
	    p_("%{}; %{} %{}", v.first, v.second, &d);
	}

	p_("-");

	for (auto [i, d]: seq::enumerate(seq::head(seq::tail(vec, 7), 5), 7)) {
	    p_("%{}: %{} %{}", i, d, &d);
	    if (i == 8)
		d = 9.99;
	}

	p_("-");

	for (auto [i, d]: seq::enumerate(seq::head(seq::tail(vec, 7), 5), 7)) {
	    p_("%{}: %{} %{}", i, d, &d);
	}

	p_("-");

	const std::vector<double>& vec2 = vec;
	//for (auto& v: seq::enumerate(vec2)) {		// error
	for (auto [i, d]: seq::enumerate(vec2)) {
	    if (i == 5)
		break;
	    p_("%{}; %{} %{}", i, d, &d);
	}
    }
}

static void parray_test()
{
    p_("-- c_parray -------------------------------------------");
    {
	struct person {
	    const char *name;
	    int age;
	};
	person *pa[] = {
	    new person{"papa",53},
	    new person{"mama", 52},
	    new person{"yuuka", 22},
	    new person{"yuuki", 26}, 
	    nullptr,
	};

	p_("- directo for -");
	for (auto p: pa) {
	    p_("%{}", (void*)p);
	}

	p_("- c_parray : *[] -");
	for (auto p: seq::c_parray(pa)) {
	    p_("%{} %{}", p->name, p->age);
	}

	person **pp = pa;
	p_("- c_parray : ** -");
	for (auto p: seq::c_parray(pp)) {
	    p_("%{} %{}", p->name, p->age);
	}
    }

    p_("-- char* array -------------------------------------------");
    {
	const char *pp[] = { "My", "name", "is", "???", nullptr };
	for (auto p: seq::charp_array(pp)) {
	    p_("%{} %{}", p.size(), p);
	}
    }
}

int main()
{
    range_test();
    combo1_test();
    combo2_test();
    enum_test();
    parray_test();
    return 0;
}
