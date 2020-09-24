#include <c7format.hpp>
#include <c7thread.hpp>

using namespace std;

#define p_ c7::p_

struct tls_a {
    int v = 777;
    tls_a() {
	p_(" tls_a: id:%{}", c7::thread::self::id());
    }
    ~tls_a() {
	p_("~tls_a: id:%{}", c7::thread::self::id());
    }
};
    
struct tls_b {
    int v = 888;
    tls_b() {
	p_(" tls_b: id:%{}", c7::thread::self::id());
    }
    ~tls_b() {
	p_("~tls_b: id:%{}", c7::thread::self::id());
    }
};
    

static thread_local tls_a tls_a_obj;
static thread_local tls_b tls_b_obj;


static void test_spinlock()
{
    p_("spinlock ----------------------------------");

    c7::thread::spinlock m;

    p_("enter block#1");
    {
	auto defer = m.lock();
	p_("in lock");
	p_("m.trylock() -> %{}", m.trylock());
    }
    p_("exit block#1");
    p_("m.trylock() -> %{}", m.trylock());
    m.unlock();

    p_("enter block#2");
    {
	m.lock_do(
	    [&]() {
		p_("in lock");
		p_("m.trylock() -> %{}", m.trylock());
	    });
	p_("out of lock_do");
	p_("m.trylock() -> %{}", m.trylock());
	m.unlock();
    }
    p_("exit block#2");
}

static void test_mutex()
{
    p_("mutex ----------------------------------");

    c7::thread::mutex m;

    p_("enter block#1");
    {
	auto defer = m.lock();
	p_("in lock");
	p_("m.trylock() -> %{}", m.trylock());
    }
    p_("exit block#1");
    p_("m.trylock() -> %{}", m.trylock());
    m.unlock();

    p_("enter block#2");
    {
	m.lock_do(
	    [&]() {
		p_("in lock");
		p_("m.trylock() -> %{}", m.trylock());
	    });
	p_("out of lock_do");
	p_("m.trylock() -> %{}", m.trylock());
	m.unlock();
    }
    p_("exit block#2");
}

static void test_thread_tls()
{
    p_("thread_tls ----------------------------------");

    c7::thread::thread th1;
    c7::thread::thread th2;
    c7::thread::thread th3;

    th1.target([](){
	    p_("th1 enter: %{}", c7::thread::self::id());
	    int v = tls_a_obj.v;
	    c7::sleep_us(1'500'000);
	    p_("th1 sleeped: %{}", v);
	    c7::thread::self::exit();
	    p_("-------------- th1 NOT REACHED HERE -------------");
	});
    th1.finalize([](){
	    p_("th1 exit: %{}", c7::thread::self::id());
	});

    th2.target([](int add){
	    p_("th2 enter: %{}", c7::thread::self::id());
	    int v = tls_b_obj.v;
	    c7::sleep_us(1'400'000 + add);
	    p_("th2 sleeped: %{}", v);
	},
	200000);
    th2.finalize([](){
	    p_("th2 exit: %{}", c7::thread::self::id());
	});

    th3.target([](){
	    p_("th3 enter: %{}", c7::thread::self::id());
	    int v = tls_a_obj.v;
	    c7::sleep_us(1'500'000);
	    p_("th3 sleeped: %{}", v);
	    c7::thread::self::abort();
	    p_("-------------- th3 NOT REACHED HERE -------------");
	});

    th1.start();
    th2.start();
    th3.start();

    th1.join();
    th2.join();
    th3.join();

    p_("th1 status: %{}", th1.status());
    p_("th2 status: %{}", th2.status());
    p_("th3 status: %{}", th3.status());

    p_("exit process");
}


int main()
{
    test_spinlock();
    test_mutex();
    test_thread_tls();
    p_("... end ...");
    return 0;
}
