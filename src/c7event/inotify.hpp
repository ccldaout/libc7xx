/*
 * c7event/inotify.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_INOTIFY_HPP_LOADED__
#define C7_EVENT_INOTIFY_HPP_LOADED__
#include <c7common.hpp>


#include <sys/inotify.h>
#include <c7event/monitor.hpp>


namespace c7::event {


class inotify_provider: public c7::event::provider_interface {
public:
    struct inotify_wfd_tag {};
    using wfd_t = simple_wrap<int, inotify_wfd_tag>;

    static constexpr const char * const manage_key = "c7event.inotify_provider";

    static c7::result<std::shared_ptr<inotify_provider>> make();
    static c7::result<std::shared_ptr<inotify_provider>> make_and_manage();
    static c7::result<std::shared_ptr<inotify_provider>> make_and_manage(c7::event::monitor&);

    ~inotify_provider() override;
    int fd() override;
    void on_event(c7::event::monitor&, int ifd, uint32_t events) override;

    c7::result<wfd_t> add_watch(const std::string& path, uint32_t mask,
				std::function<void(inotify_event&)>);

    c7::result<> rm_watch(wfd_t wd);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;

    inotify_provider();
};


} // namespace c7::event


#endif // c7event/inotify.hpp

