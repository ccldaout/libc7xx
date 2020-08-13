#include "c7format.hpp"
#include "c7thread.hpp"
#include "c7threadsync.hpp"
#include "c7threadcom.hpp"

using namespace std;

#define p_ c7::p_

static void test_fpipe(int abort_after_us = 0)
{
    p_("fpipe ----------------------------------");

    const int loop = 1000;
    const int fpsize = 23;
    const int flushby = 123;

    c7::thread::fpipe<int> fp(fpsize);

    // producer
    c7::thread::thread th1;
    th1.target([&]() {
	    p_("producer start ...");
	    for (int data = 1; data < loop; data++) {
		if (auto s = fp.put(data); s != c7::com_status::OK) {
		    p_("producer put error ... status:%{}", s);
		    return;
		}
		if ((data % flushby) == 0) {
		    if (auto s = fp.flush(); s != c7::com_status::OK) {
			p_("producer flush error ... status:%{}", s);
			return;
		    }
		}
	    }
	    p_("producer close ...");
	    if (auto s = fp.close(); s != c7::com_status::OK) {
		p_("producer close error ... status:%{}", s);
		return;
	    }
	    p_("producer closed and wait 1.5sec");
	    c7::sleep_us(1500000);
	    p_("producer end");
	});

    // consumer
    c7::thread::thread th2;
    th2.target([&]() {
	    int data = 0;
	    int idx = 0;
	    p_("consumer start ...");
	    c7::com_status s;
	    while ((s = fp.get(data)) == c7::com_status::OK) {
		if (data != ++idx) {
		    p_("consumer data error ... data:%{}, idx:%{}", data, idx);
		    return;
		}
//		p_("consumer data:%{}, idx:%{}", data, idx);
	    }
	    p_("consumer end ... last data:%{}, eof:%{}, status:%{}",
	       data, s == c7::com_status::CLOSED, s);
	});

    th2.start();
    th1.start();

    if (abort_after_us > 0) {
	c7::sleep_us(abort_after_us);
	p_("abort ...");
	fp.abort();
    }	

    th2.join();
    th1.join();
}

static void test_channel1()
{
    p_("channel#1 ----------------------------------");

    c7::com_status sts;
    c7::thread::channel<std::string, size_t> ch;
    sts = ch.send_request("string");
    p_("send_request -> %{}", sts);

    std::string req;
    sts = ch.recv_request(req);
    p_("recv_request -> %{}, <%{}>", sts, req);
    
    sts = ch.send_reply(req.size());
    p_("send_reply(%{}) -> %{}", req.size(), sts);

    size_t n;
    sts = ch.recv_reply(n);
    p_("recv_reply -> %{}, <%{}>", sts, n);
}

static void test_channel2()
{
    c7::com_status sts;
    c7::thread::channel<std::string, size_t, c7::thread::fpipe> ch(5);
    
    c7::thread::thread th;
    th.target([&]() {
	    c7::com_status sts;
	    std::string req;
	    ch.attach();
	    p_("thread: ready");
	    while ((sts = ch.recv_request(req)) == c7::com_status::OK) {
		p_("thread: req <%{}>", req);
		ch.send_reply(req.size());
	    }
	    p_("thread: sts:%{}", sts);
	    ch.close_reply();	// IMPORTANT
	    p_("thread: detach ...");
	    ch.detach();
	    p_("thread: detach ... OK");
	});

    ch.attach();
    th.start();
    c7::sleep_ms(500);

    const char *ss[] = {
	"test",
	"channel2",
	"thread",
	"and",
	"fpipe",
	"sleep",
    };
    for (auto s: ss) {
	p_("main: send_request(%{}) ...", s);
	sts = ch.send_request(std::string(s));
	p_("main: %{}", sts);
    }
    ch.close_request();		// IMPORTANT

    do {
	size_t n = 0;
	p_("main: recv_reply: ...");
	sts = ch.recv_reply(n, c7::mktimespec(2));
	p_("main: recv_reply: %{}, %{}", sts, n);
    } while (sts != c7::com_status::CLOSED && sts != c7::com_status::ABORT);

    p_("main: sleep 2");
    c7::sleep_ms(2000);
    p_("main: detach ...");
    ch.detach();
    p_("main: detach ... OK");
    th.join();
    p_("main: joined");
}

int main()
{
    test_fpipe();
    test_fpipe(5);
    test_channel1();
    test_channel2();

    p_("... end ...");
    return 0;
}
