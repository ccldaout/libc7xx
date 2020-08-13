#include "c7format.hpp"
#include "c7signal.hpp"
#include "c7threadsync.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

#define p_ c7::p_

static c7::thread::event do_exit;

static void sigint(int sig)
{
    static int rest = 5;
    p_("sig:%{}, rest:%{}", sig, rest);
    if (rest-- == 0)
	do_exit.set();
}

int main()
{
    std::string s;
    p_("SIGINT ...");
    cin >> s;

    p_("signal setup ...");
    c7::signal::handle(SIGINT, sigint);

    do_exit.wait();

    return 0;
}
