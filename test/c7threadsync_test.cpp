#include <c7format.hpp>
#include <c7threadsync.hpp>

using namespace std;

#define p_ c7::p_

static void test_group()
{
    p_("group ----------------------------------");

    c7::thread::thread th1;
    th1.target([](int ms){
	    p_("th1 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th1 exit");
	    c7::thread::self::exit();
	    p_("-------------- th1 NOT REACHED HERE -------------");
	},
	1500);
    th1.finalize([](){
	    p_("th1 finish: %{}", c7::thread::self::id());
	});

    c7::thread::thread th2;
    th2.target([](int ms){
	    p_("th2 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th2 return");
	},
	1200);
    th2.finalize([](){
	    p_("th2 finish: %{}", c7::thread::self::id());
	});

    c7::thread::thread th3;
    th3.target([](int ms){
	    p_("th3 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th3 abort");
	    c7::thread::self::abort();
	    p_("-------------- th3 NOT REACHED HERE -------------");
	},
	1600);
    th3.finalize([](){
	    p_("th3 finish: %{}", c7::thread::self::id());
	});

    c7::thread::group grp;
    grp.add(th1);
    grp.add(th2);
    grp.add(th3);

    grp.on_all_finish += [](){
	p_("on_all_finish");
    };
    p_("grp.start ...");
    grp.start();

    grp.wait();
    p_("grp.wait returned");

    for (auto th: grp) {
	p_("th#%{} status: %{}", th.id(), th.status());
    }
}

static void test_counter()
{
    p_("counter ----------------------------------");

    c7::thread::counter c(3);

    c7::thread::thread th1;
    th1.target(
	[&](int ms) {
	    p_("th1 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th1 exit");
	    c7::thread::self::exit();
	    p_("-------------- th1 NOT REACHED HERE -------------");
	},
	1500);
    th1.finalize(
	[&](auto  c) {
	    p_("th1 finish: %{}", c7::thread::self::id());
	    c->down();
	},
	&c);

    c7::thread::thread th2;
    th2.target(
	[&](int ms) {
	    p_("th2 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th2 return");
	},
	1200);
    th2.finalize(
	[&](auto c) {
	    p_("th2 finish: %{}", c7::thread::self::id());
	    c->down();
	},
	&c);

    c7::thread::thread th3;
    th3.target(
	[&](int ms) {
	    p_("th3 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th3 abort");
	    c7::thread::self::abort();
	    p_("-------------- th3 NOT REACHED HERE -------------");
	},
	1600);
    th3.finalize(
	[&](auto c) {
	    p_("th3 finish: %{}", c7::thread::self::id());
	    c->down();
	},
	&c);

    c.on_zero += []() { p_("on_zero"); };
    
    p_("th1.start ...");
    th1.start();
    p_("th2.start ...");
    th2.start();
    p_("th3.start ...");
    th3.start();

    c.wait_just(0);
    p_("return from wait");

}

static void test_counter_byref()
{
    p_("counter byref ----------------------------------");

    c7::thread::counter c(3);

    c7::thread::thread th1;
    th1.target(
	[&](int ms) {
	    p_("th1 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th1 exit");
	    c7::thread::self::exit();
	    p_("-------------- th1 NOT REACHED HERE -------------");
	},
	1500);
    th1.finalize(
	[&](c7::thread::counter& c) {
	    p_("th1 finish: %{}", c7::thread::self::id());
	    p_("th1 &c: %{p}", &c);
	    c.down();
	},
	std::ref(c));

    c7::thread::thread th2;
    th2.target(
	[&](int ms) {
	    p_("th2 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th2 return");
	},
	1200);
    th2.finalize(
	[&](auto& c) {
	    p_("th2 finish: %{}", c7::thread::self::id());
	    p_("th2 &c: %{p}", &c);
	    c.get().down();
	},
	std::ref(c));

    c7::thread::thread th3;
    th3.target(
	[&](int ms) {
	    p_("th3 enter: %{}", c7::thread::self::id());
	    c7::sleep_us(ms * 1000);
	    p_("th3 abort");
	    c7::thread::self::abort();
	    p_("-------------- th3 NOT REACHED HERE -------------");
	},
	1600);
    th3.finalize(
	[&](auto& c) {
	    p_("th3 finish: %{}", c7::thread::self::id());
	    p_("th3 &c: %{p}", &c);
	    c7::thread::counter& c2 = c;
	    c2.down();
	},
	std::ref(c));

    c.on_zero += []() { p_("on_zero"); };
    p_("&c: %{p}", &c);
    
    p_("th1.start ...");
    th1.start();
    p_("th2.start ...");
    th2.start();
    p_("th3.start ...");
    th3.start();

    c.wait_just(0);
    p_("return from wait");

}

static void test_mask()
{
    p_("mask ----------------------------------");

#define OK (1UL<<0)
#define NG (1UL<<1)
#define USER (1U<<2)
#define SYS (1U<<3)

    p_("OK  : %{b4}", OK);
    p_("NG  : %{b4}", NG);
    p_("USER: %{b4}", USER);
    p_("SYS : %{b4}", SYS);

    c7::thread::mask mask(0);

    c7::thread::thread th;
    th.target([&]() {
	    p_("thread wait_all(USER|SYS) ...");
	    auto m = mask.wait_all(USER|SYS, 0);
	    p_("thread wait: ret:%{b4} mask:%{b4}", m, mask.get());
	    c7::sleep_us(1500000);
	    p_("thread change: on:NG, off:USER");
	    mask.change(NG, USER);
	});
    th.start();

    c7::sleep_us(300000);
    p_("main on USER");
    mask.on(USER);
    c7::sleep_us(200000);
    p_("main on SYS");
    mask.on(SYS);

    p_("main wait_any(OK|NG), claer:OK|NG:");
    auto m = mask.wait_any(OK|NG, OK|NG);
    p_("main wait_any: ret:%{b4} mask:%{b4}", m, mask.get());

    th.join();

#undef OK
#undef NG
#undef USER
#undef SYS
}

static void test_rendezvous()
{
    p_("rendezvous ----------------------------------");

    c7::thread::rendezvous rdv(4);
    auto func = [](int n, decltype(rdv)& r) {
	p_("thread#%{} rendzevous ...", n);
	r.wait();
	p_("thread#%{} rendzevous ... ok", n);
    };

    c7::thread::thread th1;
    th1.target(func, 1, std::ref(rdv));

    c7::thread::thread th2;
    th2.target(func, 2, std::ref(rdv));

    c7::thread::thread th3;
    th3.target(func, 3, std::ref(rdv));

    th1.start();
    th2.start();
    th3.start();

    c7::sleep_us(1000000);
    p_("main: rendezvous ...");
    rdv.wait();
    p_("main: rendezvous ... ok");

    th1.join();
    th2.join();
    th3.join();
}


int main()
{
    test_group();
    test_counter();
    test_counter_byref();
    test_mask();
    test_rendezvous();
    p_("... end ...");
    return 0;
}
