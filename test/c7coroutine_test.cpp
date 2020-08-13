#include "c7coroutine.hpp"
#include <iostream>
#include <cstring>


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

static void tokenize(c7::coroutine& next,
		     c7::coroutine::result<std::string> s)
{
    const char *p = s.value.data();
    const char *q;
    
    std::cout << "tokenize: <" << p << ">" << std::endl;

    while ((q = ::strchr(p, ' ')) != nullptr) {
	std::string w(p, q - p);
	auto r = next.yield(w).recv<void>();
	if (!r.is_success()) {
	    next.abort();
	}
	p = q + 1;
    }

    std::string w(p);
    std::cout << "tokenize: <" << w << "> (last)" << std::endl;
    c7::drop = next.yield(w).recv<void>();
    
    std::cout << "tokenize: end" << std::endl;
}

static void showtoken(c7::coroutine& next,
		      c7::coroutine::result<std::string> r)
{
    std::cout << "showtoken: start" << std::endl;

    while (r.is_success()) {
	std::cout << "showtoken: <" << r.value << ">" << std::endl;
	r = next.yield().recv<std::string>();
    }

    std::cout << "showtoken: end" << std::endl;
}

static void putsep(c7::coroutine& next,
		   c7::coroutine::result<void> r)
{
    std::cout << "putsep: start" << std::endl;

    while (r.is_success()) {
	std::cout << "---" << std::endl;
	r = next.yield().recv<void>();
    }

    std::cout << "putsep: end" << std::endl;
}

static void test_coroutine()
{
    auto self = c7::coroutine::self();
    c7::coroutine co1(1024);
    c7::coroutine co2(1024);
    c7::coroutine co3(1024);
    c7::coroutine co_dummy(1024);

    std::cout << "test_coroutine start" << std::endl;
    {
	co1.assign<std::string>(co2, tokenize);
	co2.assign<std::string>(co3, showtoken);
	co3.assign<void>(*self, putsep);

	auto s = std::string("This is C++ programming language");
	auto r = co1.yield(s).recv<void>();
	while (r.is_success()) {
	    r = co1.yield().recv<void>();
	}
    }
    std::cout << "test_coroutine end" << std::endl;

    std::cout << "------------------------------" << std::endl;

    std::cout << "test_coroutine start" << std::endl;
    {
	co1.assign<std::string>(co2,
				[](c7::coroutine& next,
				   c7::coroutine::result<std::string> r) {
				    tokenize(next, r);
				});

	std::function<void(c7::coroutine&,
			   c7::coroutine::result<std::string>)> fo2 =
	    [&](c7::coroutine&,		// unuse parameter
		c7::coroutine::result<std::string> r) {
	    showtoken(co3, r);		// capture
	};
	co2.assign(co_dummy, fo2);	// co_dummy pased to fo2, but fo2 ignore and use co3 captured.

	std::function<void(c7::coroutine&,
			   c7::coroutine::result<void>)> fo3 =
	    [=](c7::coroutine&,
		c7::coroutine::result<void> r) {
	    putsep(*self, r);
	};
	co3.assign(co_dummy, std::move(fo3));

	auto s = std::string("This is C++ programming language");
	auto r = co1.yield(s).recv<void>();
	while (r.is_success()) {
	    r = co1.yield().recv<void>();
	}
    }
    std::cout << "test_coroutine end" << std::endl;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

static void test_generator()
{
    std::cout << "------------------------------" << std::endl;
    std::cout << "test_generator start" << std::endl;

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

	for (auto s: gen) {
	    std::cout << "<" << s << ">" << std::endl;
	}
	std::cout << "status: " << gen.status() << std::endl;
    }
    std::cout << "------------------------------" << std::endl;

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

	for (auto s: gen) {
	    std::cout << "<" << s << ">" << std::endl;
	    if (i++ == 2)
		break;
	}
	std::cout << "status: " << gen.status() << std::endl;
    }

    std::cout << "test_generator end" << std::endl;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

int main()
{
    test_coroutine();
    test_generator();
    return 0;
}
