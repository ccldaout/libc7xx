/*
 * c7thread/datapara.hpp
 *
 * Copyright (c) 2024 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_THREAD_DATAPARA_HPP_LOADED_
#define C7_THREAD_DATAPARA_HPP_LOADED_
#include <c7common.hpp>


#include <vector>
#include <c7thread/thread.hpp>
#include <c7thread/mask.hpp>


namespace c7::thread::datapara {


static inline constexpr int MAX_N_THREAD = 30;

static inline constexpr uint64_t M_FINISH_REQ   = (1UL << 63);
static inline constexpr uint64_t M_FINISHED_ANY = (1UL << 62);

static inline constexpr uint64_t mask_start_req(int th_idx)
{
    return (1UL << (th_idx * 2 + 0));
}

static inline constexpr uint64_t mask_paused(int th_idx)
{
    return (1UL << (th_idx * 2 + 1));
}


struct configure {
    int mt_threshold;
    int n_thread;
    size_t n_item_per_thread;
};


template <typename Derived, typename ItemIn, typename ItemOut>
class driver_for_main {
public:
    driver_for_main() {}

protected:
    void init(size_t max_items) {
	in_items_.reserve(max_items);
	in_items_.clear();
	out_items_.reserve(max_items);
	out_items_.clear();
    }

    void swap_rcv(std::vector<ItemIn>& rcv_items) {
	std::swap(rcv_items, in_items_);
    }

    bool is_empty() {
	return in_items_.empty();
    }

    void apply_main_process(int th_idx, int n_thread) {
	size_t n = in_items_.size();
	out_items_.resize(n);
	for (size_t i = th_idx; i < n; i += n_thread) {
	    static_cast<Derived*>(this)->datapara_main_process(th_idx,
								in_items_[i],
								out_items_[i]);
	}
    }

    void swap_snd(std::vector<ItemOut>& snd_items) {
	std::swap(out_items_, snd_items);
    }

private:
    std::vector<ItemIn> in_items_;
    std::vector<ItemOut> out_items_;
};


template <typename Derived, typename Item>
class driver_for_main<Derived, Item, Item> {
public:
    driver_for_main() {}

protected:
    void init(size_t max_items) {
	items_.reserve(max_items);
	items_.clear();
    }

    bool is_empty() {
	return items_.empty();
    }

    void swap_rcv(std::vector<Item>& rcv_items) {
	std::swap(rcv_items, items_);
    }

    void apply_main_process(int th_idx, int n_thread) {
	size_t n = items_.size();
	for (size_t i = th_idx; i < n; i += n_thread) {
	    static_cast<Derived*>(this)->datapara_main_process(th_idx,
								items_[i]);
	}
    }

    void swap_snd(std::vector<Item>& snd_items) {
	std::swap(snd_items, items_);
    }

private:
    std::vector<Item> items_;
};


// [Requirement]
//
// const char *Derived::datapara_name();
// void Derived::datapara_main_process(int th_idx, ItemIn&, ItemOut&);	// ItemIn != ItemOut
// void Derived::datapara_main_process(int th_idx, ItemIn&);		// ItemIn == ItemOut
// void Derived::datapara_post_process(ItemOut&);
// void Derived::datapara_logger(const char *src_name, int src_line,
//	    		          uint32_t level, const std::string& s);
//
template <typename Derived, typename ItemIn, typename ItemOut = ItemIn>
class driver: public driver_for_main<Derived, ItemIn, ItemOut> {
public:
    void init(const configure& cfg);
    void start_round();
    inline c7::result<> put(const ItemIn& item);
    c7::result<> end_round();
    void end();

private:
    using driver_base = driver_for_main<Derived, ItemIn, ItemOut>;

    size_t max_items_;
    std::vector<ItemIn> rcv_items_;
    std::vector<ItemOut> snd_items_;

    c7::thread::mask sync_mask_{0};

    int main_n_thread_;
    std::vector<c7::thread::thread> ths_;

    uint64_t m_all_paused_;		// include post_thread
    uint64_t m_allmain_start_req_;	// exclude post_thread
    uint64_t m_allmain_paused_;		// exclude post_thread
    size_t   main_mt_threshold_;

    c7::result<> wait_process();
    void resume_process();
    void main_thread(const int th_idx);
    void post_thread(const int th_idx);
    const char *name() {
	return static_cast<Derived*>(this)->datapara_name();
    }
    template <typename... Args>
    void logger(const char *src_name, int src_line,
		uint32_t level, const char *format, const Args&... args) {
	static_cast<Derived*>(this)->datapara_logger(src_name, src_line, level,
						      c7::format(format, args...));
    }
};


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::init(const configure& cfg)
{
    main_n_thread_     = std::min(cfg.n_thread, MAX_N_THREAD);
    main_mt_threshold_ = cfg.mt_threshold * main_n_thread_;

    max_items_ = cfg.n_item_per_thread * main_n_thread_;
    rcv_items_.reserve(max_items_);
    rcv_items_.clear();
    snd_items_.reserve(max_items_);
    snd_items_.clear();

    driver_base::init(max_items_);

    m_allmain_start_req_ = 0;
    m_allmain_paused_	 = 0;
    for (int i = 0; i < main_n_thread_; i++) {
	m_allmain_start_req_ |= mask_start_req(i);
	m_allmain_paused_    |= mask_paused(i);
    }
    m_all_paused_    = (m_allmain_paused_ |
			mask_paused(main_n_thread_));	// for post_thread

    sync_mask_.clear();

    ths_.clear();
    for (int i = 0; i < main_n_thread_; i++) {
	c7::thread::thread th;
	th.target([this](int th_idx){ main_thread(th_idx); }, i);
	th.set_name(c7::format("main_process#%{}", i));
	th.start();
	ths_.push_back(std::move(th));
    }
    c7::thread::thread th;
    th.target([this](){ post_thread(main_n_thread_); });
    th.set_name("post_process");
    th.start();
    ths_.push_back(std::move(th));
}


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::start_round()
{
    rcv_items_.clear();
}


template <typename Derived, typename ItemIn, typename ItemOut>
c7::result<>
driver<Derived, ItemIn, ItemOut>::put(const ItemIn& item)
{
    if (rcv_items_.size() == max_items_) {
	if (auto res = wait_process(); !res) {
	    return res;
	}
	resume_process();
    }
    rcv_items_.push_back(item);
    return c7result_ok();
}


template <typename Derived, typename ItemIn, typename ItemOut>
c7::result<>
driver<Derived, ItemIn, ItemOut>::end_round()
{
    auto res = wait_process();
    if (res) {
	resume_process();
	res << wait_process();
    }
    if (res) {
	sync_mask_.wait_all(m_all_paused_, 0);
    }
    return res;
}


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::end()
{
    sync_mask_.on(M_FINISH_REQ);
    for (auto& th: ths_) {
	th.join();
	logger(__FILE__, __LINE__, C7_LOG_DTL,
	       "%{}::end: %{} is joined", name(), th.name());
    }
}


template <typename Derived, typename ItemIn, typename ItemOut>
c7::result<>
driver<Derived, ItemIn, ItemOut>::wait_process()
{
    const uint64_t m_except = M_FINISHED_ANY;
    uint64_t ret = sync_mask_.wait_all_or(m_allmain_paused_, m_except, 0);
    if ((ret & m_except) != 0) {
	logger(__FILE__, __LINE__, C7_LOG_INF,
	       "%{}::wait_process: detect M_FINISHED_ANY (unexpectedly)", name());
	return c7result_err(EFAULT, "%{}::wait_process: som post_thread is unexpectedly finished.",
			    name());
    }
    return c7result_ok();
}


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::resume_process()
{
    // resume main_thread
    driver_base::swap_rcv(rcv_items_);
    sync_mask_.off(m_allmain_paused_);		// (A) [IMPORTANT]
    sync_mask_.on(m_allmain_start_req_);
    // make sure all main_thread has been resumed.
    sync_mask_.wait_alloff(m_allmain_start_req_, 0);

    rcv_items_.clear();
}


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::main_thread(const int th_idx)
{
    c7::defer on_exit{[this, th_idx]() {
			  uint64_t m = (M_FINISHED_ANY|
					mask_paused(th_idx));
			  sync_mask_.on(m);
		      }};

    const uint64_t m_my_start_req = mask_start_req(th_idx);
    const uint64_t m_my_paused	  = mask_paused(th_idx);
    const uint64_t m_except	  = M_FINISH_REQ|M_FINISHED_ANY;

    const int      snd_idx         = main_n_thread_;
    const uint64_t m_snd_start_req = mask_start_req(snd_idx);
    const uint64_t m_snd_paused    = mask_paused(snd_idx);

    const uint64_t m_wait_others = (m_allmain_paused_ & ~m_my_paused) | m_snd_paused;

    for (;;) {
	sync_mask_.on(m_my_paused);
	uint64_t ret = sync_mask_.wait_any(m_my_start_req|m_except,
					   m_my_start_req|m_my_paused);	// clear at return
	if ((ret & m_except) != 0) {
	    auto what = (ret & M_FINISH_REQ) ? "M_FINISH_REQ" : "M_FINISHED_ANY";
	    logger(__FILE__, __LINE__, C7_LOG_INF, "%{}::main_thread#%{}: detect %{}",
		   name(), th_idx, what);
	    return;
	}

	if (driver_base::is_empty()) {
	    continue;
	}

	driver_base::apply_main_process(th_idx, main_n_thread_);

	if (th_idx == 0) {
	    // wait all other main_thread and post_thread are paused.
	    ret = sync_mask_.wait_all_or(m_wait_others, m_except, 0);
	    // It's guranteed that all main_thread has been run and paused.
	    // by calling sync_mask_.off(ma_allmain_paused_) on resume_process (A).

	    if ((ret & m_except) != 0) {
		auto what = (ret & M_FINISH_REQ) ? "M_FINISH_REQ" : "M_FINISHED_ANY";
		logger(__FILE__, __LINE__, C7_LOG_INF, "%{}::main_thread#%{}: detect %{}",
		       name(), th_idx, what);
		return;
	    }

	    // resume post_thread
	    driver_base::swap_snd(snd_items_);
	    sync_mask_.on(m_snd_start_req);
	    // make sure post_thread has been resumed.
	    sync_mask_.wait_alloff(m_snd_start_req, 0);
	}
    }
}


template <typename Derived, typename ItemIn, typename ItemOut>
void
driver<Derived, ItemIn, ItemOut>::post_thread(const int th_idx)
{
    const uint64_t m_my_start_req = mask_start_req(th_idx);
    const uint64_t m_my_paused = mask_paused(th_idx);
    const uint64_t m_except = M_FINISH_REQ|M_FINISHED_ANY;

    c7::defer on_exit{[this, th_idx]() {
			  uint64_t m = (M_FINISHED_ANY|
					mask_paused(th_idx));
			  sync_mask_.on(m);
		      }};

    for (;;) {
	sync_mask_.on(m_my_paused);
	uint64_t ret = sync_mask_.wait_any(m_my_start_req|m_except, m_my_start_req|m_my_paused);
	if ((ret & m_except) != 0) {
	    auto what = (ret & M_FINISH_REQ) ? "M_FINISH_REQ" : "M_FINISHED_ANY";
	    logger(__FILE__, __LINE__, C7_LOG_INF, "%{}::post_thread  : detect %{}",
		   name(), what);
	    return;
	}

	for (auto& data: snd_items_) {
	    static_cast<Derived*>(this)->datapara_post_process(data);
	}
    }
}


} // namespace c7::thread::datapara


#endif // c7thread/datapara.hpp
