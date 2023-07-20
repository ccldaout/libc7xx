/*
 * c7event/inotify.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <sys/eventfd.h>
#include <c7fd.hpp>
#include <c7event/inotify.hpp>


namespace std {
template <>
struct hash<c7::event::inotify_provider::wfd_t> {
    auto operator()(const c7::event::inotify_provider::wfd_t &wfd) const {
	return hash<int>{}(wfd());
    }
};
}


namespace c7::event {


class inotify_provider::impl {
public:
    impl();
    ~impl() = default;
    int fd() { return ifd_; }
    void on_event(c7::event::monitor&, int prvfd, uint32_t events);

    c7::result<wfd_t> add_watch(const std::string& path, uint32_t mask,
				std::function<void(inotify_event&)>);

    c7::result<> rm_watch(wfd_t wd);

private:
    c7::fd ifd_;
    std::unordered_map<wfd_t, std::function<void(inotify_event&)>> cb_;
};


inotify_provider::impl::impl(): ifd_(inotify_init())
{
}


void
inotify_provider::impl::on_event(c7::event::monitor& mon, int ifd, uint32_t)
{
    alignas(long) char buffer[8192];
    static_assert(sizeof(buffer) > (sizeof(inotify_event) + NAME_MAX));

    auto n = read(ifd, buffer, sizeof(buffer));
    if (n <= 0) {
	// oops!
	ifd_.close();
	return;
    }
    char *p = buffer;
    do {
	auto inev = reinterpret_cast<inotify_event*>(p);
	if (auto it = cb_.find(inev->wd); it == cb_.end()) {
	    // error ?
	    rm_watch(inev->wd);
	} else {
	    if (inev->len == 0) {
		inev->name[0] = 0;
	    }
	    (*it).second(*inev);
	}
	auto size = sizeof(*inev) + inev->len;
	p += size;
	n -= size;
    } while (n > 0);
}


c7::result<inotify_provider::wfd_t>
inotify_provider::impl::add_watch(const std::string& path, uint32_t mask,
				  std::function<void(inotify_event&)> callback)
{
    auto wd = inotify_add_watch(ifd_, path.c_str(), mask);
    if (wd == C7_SYSERR) {
	return c7result_err(errno,
			    "inotify_add_watch(i:%{}, %{}, m:0x%{x}) failed",
			    ifd_, path, mask);
    }

    c7::defer on_error{[this, wd](){ rm_watch(wd); }};
    cb_.insert_or_assign(wd, callback);
    on_error.cancel();

    return c7result_ok(wfd_t{wd});
}


c7::result<>
inotify_provider::impl::rm_watch(wfd_t wfd)
{
    cb_.erase(wfd);
    if (inotify_rm_watch(ifd_, wfd) == C7_SYSERR) {
	return c7result_err(errno,
			    "inotify_rm_watch(i:%{}, w:%{}) failed", ifd_, wfd());
    }
    return c7result_ok();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

inotify_provider::inotify_provider(): pimpl_(new impl)
{
}

inotify_provider::~inotify_provider() = default;

int
inotify_provider::fd()
{
    return pimpl_->fd();
}

void
inotify_provider::on_event(c7::event::monitor& mon, int ifd, uint32_t events)
{
    pimpl_->on_event(mon, ifd, events);
}

c7::result<inotify_provider::wfd_t>
inotify_provider::add_watch(const std::string& path, uint32_t mask,
			    std::function<void(inotify_event&)> callback)
{
    return pimpl_->add_watch(path, mask, callback);
}

c7::result<>
inotify_provider::rm_watch(wfd_t wd)
{
    return pimpl_->rm_watch(wd);
}

c7::result<std::shared_ptr<inotify_provider>>
inotify_provider::make()
{
    return c7result_ok(std::shared_ptr<inotify_provider>(new inotify_provider));
}

c7::result<std::shared_ptr<inotify_provider>>
inotify_provider::make_and_manage()
{
    return make_and_manage(c7::event::default_event_monitor());
}

c7::result<std::shared_ptr<inotify_provider>>
inotify_provider::make_and_manage(c7::event::monitor& mon)
{
    if (auto res = make(); !res) {
	return res;
    } else {
	auto sp = std::move(res.value());
	if (auto res = mon.manage(manage_key, sp); !res) {
	    return res.as_error();
	}
	return c7result_ok(sp);
    }
}


} // namespace c7::event
