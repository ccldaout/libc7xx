/*
 * c7event/submit.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_SUBMIT_HPP_LOADED__
#define C7_EVENT_SUBMIT_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7fd.hpp>
#include <c7thread/queue.hpp>


namespace c7::event {


class submit_provider: public c7::event::provider_interface {
public:
    static constexpr const char * const manage_key = "c7event.submit_provider";

    c7::delegate<void, c7::result_base&> on_error;

    static c7::result<std::shared_ptr<submit_provider>> make();
    static c7::result<std::shared_ptr<submit_provider>> make_and_manage();
    static c7::result<std::shared_ptr<submit_provider>> make_and_manage(c7::event::monitor&);

    ~submit_provider() override = default;
    int fd() override { return evfd_; }
    void on_event(c7::event::monitor&, int prvfd, uint32_t events) override;

    c7::result<> submit(std::function<void()>&& f);

private:
    c7::fd evfd_;
    c7::thread::queue<std::function<void()>> callbacks_;

    submit_provider() = default;
    c7::result<> init();
};


} // namespace c7::event


#endif // c7event/submit.hpp

