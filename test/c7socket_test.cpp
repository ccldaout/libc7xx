#include <c7socket.hpp>
#include <c7thread.hpp>
#include <c7format.hpp>

#define p_	c7::p_


static void server(c7::socket& svr)
{
    p_("server: %{}", svr);
    auto res2 = svr.accept();
    if (!res2) {
	p_("%{}", res2);
	return;
    }
    auto sock = std::move(res2.value());

    auto defer = c7::defer([&](){ sock.close(); });

    if (auto res = sock.write("data", 5); !res) {
	p_("%{}", res);
	return;
    }

    c7::sleep_us(1000000);

    if (auto res = sock.write("end", 4); !res) {
	p_("%{}", res);
	return;
    }
}


static void test1()
{
    auto res = c7::tcp_server("", 12345);
    if (!res) {
	p_("%{}", res);
	return;
    }
    auto svr = std::move(res.value());

    c7::thread::thread th;
    th.target(server, std::ref(svr));
    th.start();

    res = c7::tcp_client("localhost", 12345);
    if (!res) {
	p_("%{}", res);
	return;
    }
    auto sock = std::move(res.value());
    
    char buff[512];

    sock.set_nonblocking(true);
    auto r = sock.read_n(buff, 10);
    p_("%{} <%{}>", r, buff);

    sock.set_nonblocking(true);
    r = sock.read_n(buff, 10);
    p_("%{} <%{}>", r, buff);

    sock.set_nonblocking(false);
    r = sock.read_n(buff, 10);
    p_("%{} <%{}>", r, buff);

    r = sock.read_n(buff, 10);
    p_("%{} <%{}>", r, buff);

    sock.close();
}



int main()
{
    test1();
    return 0;
}
