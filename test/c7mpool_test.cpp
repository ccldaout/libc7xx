#include <c7format.hpp>
#include <c7mpool.hpp>
#include <c7threadcom.hpp>
#include <c7utils.hpp>

#define p_	c7::p_


struct data_t {
    static int age_n;
    int age;
    void *addr;
    data_t () {
	age = data_t::age_n++;
	addr = nullptr;
	p_("data_t: age:%{}, %{}", age, addr);
    }
    ~data_t() {
	p_("~data_t: age:%{}, %{}", age, addr);
    }
};
int data_t::age_n = 0;


void test_1()
{
    p_("\n-------- [ test 1 ] --------");

    auto mp = c7::mpool_waitable<data_t>(5, 2);
    std::vector<c7::mpool_ptr<data_t>> mv;
    std::vector<c7::mpool_ptr<data_t>> mv2;

    for (int i = 0; i < 8; i++) {
	auto d = mp.get();
	d->age = i;
	d->addr = d.get();
	mv.push_back(std::move(d));
    }

    c7::thread::thread th;
    th.target([&]() {
	    p_("thread");
	    c7::sleep_ms(100);
	    for (auto&& d: mv) {
		auto dd = std::move(d);
		p_("thread: age:%{}, %{}", dd->age, dd->addr);
	    }
	    p_("thread ... end");
	});
    th.start();

    for (int i = 0; i < 5; i++) {
	p_("main ...");
	auto d = mp.get();
	p_("main ... age:%{}, %{}", d->age, d->addr);
	mv2.push_back(std::move(d));
    }
    p_("sizeof(mp.get(): %{}", sizeof(mp.get()));

    mv2.clear();

    mp.set_strategy(c7::mpool::chunk_strategy<data_t>::make, 10);
}


void test_2()
{
    p_("\n-------- [ test 2 ] --------");

    auto fp = c7::thread::fpipe<c7::mpool_ptr<data_t>>(4);
    c7::thread::thread th;
    th.target([&]() {
	    p_("- fpipe::get -");
	    for (int i = 0; i < 2; i++) {
		p_("fp-> #%{}", i);
		c7::mpool_ptr<data_t> pp;
		fp.get(pp);
		p_("fp -> age:%{}, %{}", pp->age, pp->addr);
	    }
	});
    th.start();

    {
	p_("-");
	auto mp = c7::mpool_chunk<data_t>(5);
	p_("-");
	for (int i = 0; i < 7; i++) {
	    p_("mp->fp #%{}", i);
	    auto d = mp.get();
	    d->age = 10 + i;
	    d->addr = static_cast<void*>(&*d);
	    fp.put(std::move(d));
	}
	p_("mp destruct ...");
    }
    th.join();

    p_("fp destruct ...");
}


void test_3()
{
    p_("\n-------- [ test 3 ] --------");

    auto fp = c7::thread::fpipe<c7::mpool_ptr<data_t>>(4);
    c7::thread::thread th;
    th.target([&]() {
	    p_("- fpipe::get -");
	    for (int i = 0; i < 2; i++) {
		p_("fp-> #%{}", i);
		c7::mpool_ptr<data_t> pp;
		fp.get(pp);
		p_("fp -> age:%{}, %{}", pp->age, pp->addr);
	    }
	});
    th.start();

    {
	p_("-");
	auto mp = c7::mpool_chunk<data_t>(5);
	p_("-");
	for (int i = 0; i < 7; i++) {
	    p_("mp->fp #%{}", i);
	    auto d = mp.get();
	    d->age = 10 + i;
	    d->addr = static_cast<void*>(&*d);
	    fp.put(std::move(d));
	}
	p_("mp destruct ...");
    }
    th.join();
    p_("fp.reset");
    fp.reset();			// reset
    p_("fp destruct ...");
}


int main()
{
    test_1();
    test_2();
    test_3();

    p_("--- main end ---");
    return 0;
}
