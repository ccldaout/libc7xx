#include <c7format.hpp>
#include <c7mpool.hpp>
#include <c7utils.hpp>

#define p_	c7::p_


struct data_t {
    int age;
    void *addr;
};


int main()
{
    auto mp = c7::mpool_waitable<data_t>(5, 2);
    std::vector<c7::mpool_ptr<data_t>> mv;
    std::vector<decltype(mp)::ptr> mv2;

    for (int i = 0; i < 8; i++) {
	auto d = mp.get();
	d->age = i;
	d->addr = d.get();
	mv.push_back(std::move(d));
    }

    c7::thread::thread th;
    th.target([&]() {
	    p_("thread");
	    c7::sleep_ms(1000);
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

    return 0;
}
