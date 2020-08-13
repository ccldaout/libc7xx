
#include "c7ring.hpp"
#include "c7seq.hpp"
#include "c7format.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <typeinfo>

using namespace std;


struct data_t {
    int v;
    int w;

    data_t(int v, int w): v(v), w(w) {}

    data_t (const data_t& o): v(o.v), w(o.w) {}

    data_t(data_t&& o): v(o.v), w(o.w) {
	debug("data_t(&&)");
	o.v = o.w = 99;
    }
};

#define DATA 1

#if DATA == 0
data_t make_data(int v)
{
    return data_t(v, v*2);
}
#elif DATA == 1
data_t& make_data(int v)
{
    return *(new data_t(v, v*2));
}
#elif DATA == 2
data_t* make_data(int v)
{
    return new data_t(v, v*2);
}
#elif DATA == 3
auto make_data(int v)
{
    return std::unique_ptr<data_t>(new data_t(v, v*2));
}
#elif DATA == 4
auto make_data(int v)
{
    return std::shared_ptr<data_t>(new data_t(v, v*2));
}
#endif


int main()
{
    {
	auto root = c7::make_ring(make_data(0));
	for (auto v: c7::seq::range(9, 1)) {
	    auto n = c7::new_ring(make_data(v));
	    root.insert_prev(*n);
	}
	for (auto& n: root)
	    c7::p_("%{}", n->v);
	debug("--");
	for (auto& n: c7::seq::reverse(root))
	    c7::p_("%{}", n->v);
    }

    debug("----------------------------------------------------");

    {
	auto r1 = c7::new_ring(make_data(0));
	for (auto v: c7::seq::range(2, 1)) {
	    auto n = c7::new_ring(make_data(v));
	    r1->insert_prev(*n);
	}
	for (auto v: c7::seq::range(3, 6)) {
	    auto n = c7::new_ring(make_data(v));
	    r1->insert_prev(*n);
	}
	for (auto& n: *r1)
	    c7::p_("%{}", n->v);
	debug("--");

	auto r2 = c7::new_ring(make_data(3));
	for (auto v: c7::seq::range(2, 4)) {
	    auto n = c7::new_ring(make_data(v));
	    r2->insert_prev(*n);
	}
	for (auto& n: *r2)
	    c7::p_("%{}", n->v);
	debug("--");

	auto& c = r1->next().next();
	c7::p_("cur: %{}", c->v);
	c.insert_next(*r2);
	for (auto& n: *r1)
	    c7::p_("%{}", n->v);
	debug("--");
	for (auto& n: *r2)
	    c7::p_("%{}", n->v);
    }

    debug("----------------------------------------------------");

    {
	auto r1 = c7::new_ring(make_data(0));
	for (auto v: c7::seq::range(2, 1)) {
	    auto n = c7::new_ring(make_data(v));
	    r1->insert_prev(*n);
	}
	for (auto v: c7::seq::range(3, 6)) {
	    auto n = c7::new_ring(make_data(v));
	    r1->insert_prev(*n);
	}
	for (auto& n: *r1)
	    c7::p_("%{}", n->v);
	debug("--");

	auto r2 = c7::new_ring(make_data(3));
	for (auto v: c7::seq::range(2, 4)) {
	    auto n = c7::new_ring(make_data(v));
	    r2->insert_prev(*n);
	}
	for (auto& n: *r2)
	    c7::p_("%{}", n->v);
	debug("--");

	auto& c = r1->next().next().next();
	c7::p_("cur: %{}", c->v);
	c.insert_prev(*r2);
	for (auto& n: *r1)
	    c7::p_("%{}", n->v);
	debug("--");
	for (auto& n: *r2)
	    c7::p_("%{}", n->v);
    }

    debug("----------------------------------------------------");

    {
	auto r = c7::new_ring(make_data(0));
	for (auto v: c7::seq::range(9, 1)) {
	    auto n = c7::new_ring(make_data(v));
	    r->insert_prev(*n);
	}
	auto& n = r->next().next().next();
	n.unlink();
	c7::p_("unlink: %{}", n->v);
	for (auto& n: *r)
	    c7::p_("%{}", n->v);
    }

    debug("----------------------------------------------------");

    {
	auto r = c7::new_ring(make_data(0));
	for (auto v: c7::seq::range(9, 1)) {
	    auto n = c7::new_ring(make_data(v));
	    r->insert_prev(*n);
	}
	auto& n1 = r->next().next().next();
	auto& n2 = n1.next().next();
	c7::p_("unlink: %{}..%{}", n1->v, n2->v);

	n1.unlink(n2);
	for (auto& n: n1)
	    c7::p_("%{}", n->v);
	debug("--");
	for (auto& n: *r)
	    c7::p_("%{}", n->v);

	debug("-- move --");
	c7::p_("(*n1)->v %{}", (*n1).v);
	//auto p = std::move(n1.ref());
	auto p = n1.move();
#if DATA < 2
	c7::p_("n1 -> p %{}", p.v);
#else
	c7::p_("n1 -> p %{}", p->v);
#endif
	c7::p_("n1->v %{}", n1->v);		// SIGSEGV on unique_ptr / shared_ptr
    }

    return 0;
}
