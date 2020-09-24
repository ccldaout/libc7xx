#include <c7mlog.hpp>
#include <c7seq.hpp>
#include <c7threadsync.hpp>
#include <c7utils.hpp>
#include <cstring>

#define p_	c7::p_

static void thread_test()
{
    const int duration_ms = 500;

    p_("-- thread ----------------------------------------------");

    uint32_t flags = (C7_MLOG_F_THREAD_NAME |
		      C7_MLOG_F_SOURCE_NAME |
		      C7_MLOG_F_SPINLOCK);

    c7::mlog_writer mlog;
    auto res = mlog.init("c7xxdev", 128, 1024*1024, flags);
    if (!res) {
	p_("writer: %{}", res);
	return;
    }
    (void)std::strcpy((char *)mlog.hdraddr(), "c7xxdev");

    c7::thread::group grp;
    std::vector<c7::thread::thread> thv(3);
    static std::atomic<bool> stop_thread = false;

    std::function<void(int, std::string)>
	logger([&](auto i, auto name) {
		for (size_t n = 0;; n++) {
		    size_t mdata = (((size_t)i)<<32)|n;
		    mlog.format(__FILE__, __LINE__, 0, 0, mdata, "%{}: %{}", name, n);
		    if (stop_thread) {
			p_("%{} stopped at %{}", name, n);
			return;
		    }
		}
	    });

    for (auto [i, th]: c7::seq::enumerate(thv)) {
	grp.add(th);
	th.target(logger, i, c7::format("thread#%{}", i));
	th.start();
    }

    grp.start();
    c7::sleep_us(duration_ms * 1000);
    stop_thread = true;
    grp.wait();

    p_("-- scan --");
    c7::mlog_reader reader;
    res = reader.load("c7xxdev");
    if (!res) {
	p_("reader: %{}", res);
	return;
    }

    for (int i = 0; i < (int)thv.size(); i++) {
	size_t mdata = 0;
	uint64_t order;
	p_("-- scan %{} --", i);
	reader.scan(0, 0, 0,
		    [&](const c7::mlog_reader::info_t& info) {
			return ((info.minidata >> 32) == (size_t)i);
		    },
		    [&](const c7::mlog_reader::info_t& info, void *) {
			if (mdata == 0) {
			    order = info.order;
			    mdata = (info.minidata << 32) >> 32;
			    return true;
			} else {
			    size_t o_mdata = mdata;
			    uint64_t o_order = order;
			    mdata = (info.minidata << 32) >> 32;
			    order = info.order;
			    if (o_mdata + 1 == mdata)
				return true;
			    p_("#%{}:%{} -> #%{}:%{}", o_order, o_mdata, order, mdata);
			    return false;
			}
		    });
    }
}


int main()
{
    thread_test();
    return 0;
}
