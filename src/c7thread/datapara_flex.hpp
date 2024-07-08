/*
 * c7thread/datapara_flex.hpp
 *
 * Copyright (c) 2024 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_THREAD_DATAPARA_FLEX_HPP_LOADED_
#define C7_THREAD_DATAPARA_FLEX_HPP_LOADED_
#include <c7common.hpp>


#include <c7thread/datapara.hpp>


namespace c7::thread::datapara {


template<typename Item>
class flex_driver {
public:
    virtual ~flex_driver() {}
    virtual void init(const configure& cfg) = 0;
    virtual void start_round() = 0;
    virtual c7::result<> put(const Item& item) = 0;
    virtual c7::result<> end_round() = 0;
    virtual void end() = 0;
};


template <typename...>
class flex_driver_impl;


template<typename Item,
	 typename Main, typename Post, typename Logger>
class flex_driver_impl<Item, Main, Post, Logger>:
	public flex_driver<Item>,
	public driver<flex_driver_impl<Item, Main, Post, Logger>, Item> {
private:
    using driver_t = driver<flex_driver_impl<Item, Main, Post, Logger>, Item>;

    const char *n_;
    Main mf_;
    Post pf_;
    Logger lg_;

public:
    flex_driver_impl(const char *n, Main mf, Post pf, Logger lg):
	n_(n), mf_(mf), pf_(pf), lg_(lg) {
    }

    void init(const configure& cfg) override {
	driver_t::init(cfg);
    }
    void start_round() override {
	driver_t::start_round();
    }
    c7::result<> put(const Item& item) override {
	return driver_t::put(item);
    }
    c7::result<> end_round() override {
	return driver_t::end_round();
    }
    void end() override {
	driver_t::end();
    }

    // for dirver<...>

    const char *datapara_name() {
	return n_;
    }

    void datapara_main_process(int th_idx, Item& v) {
	mf_(th_idx, v);
    }

    void datapara_post_process(Item& v) {
	pf_(v);
    }

    void datapara_logger(const char *src_name, int src_line,
			 uint32_t level, const std::string& s) {
	lg_(src_name, src_line, level, s);
    }
};


template<typename ItemIn, typename ItemOut,
	 typename Main, typename Post, typename Logger>
class flex_driver_impl<ItemIn, ItemOut, Main, Post, Logger>:
	public flex_driver<ItemIn>,
	public driver<flex_driver_impl<ItemIn, ItemOut, Main, Post, Logger>, ItemIn, ItemOut> {
private:
    using driver_t = driver<flex_driver_impl<ItemIn, ItemOut, Main, Post, Logger>, ItemIn, ItemOut>;

    const char *n_;
    Main mf_;
    Post pf_;
    Logger lg_;

public:
    flex_driver_impl(const char *n, Main mf, Post pf, Logger lg):
	n_(n), mf_(mf), pf_(pf), lg_(lg) {
    }

    void init(const configure& cfg) override {
	driver_t::init(cfg);
    }
    void start_round() override {
	driver_t::start_round();
    }
    c7::result<> put(const ItemIn& item) override {
	return driver_t::put(item);
    }
    c7::result<> end_round() override {
	return driver_t::end_round();
    }
    void end() override {
	driver_t::end();
    }

    // for dirver<...>

    const char *datapara_name() {
	return n_;
    }

    void datapara_main_process(int th_idx, ItemIn& in, ItemOut& out) {
	mf_(th_idx, in, out);
    }

    void datapara_post_process(ItemOut& v) {
	pf_(v);
    }

    void datapara_logger(const char *src_name, int src_line,
			 uint32_t level, const std::string& s) {
	lg_(src_name, src_line, level, s);
    }
};


template <typename Item,
	  typename Main, typename Post, typename Logger>
static inline std::unique_ptr<flex_driver<Item>>
make_driver(const char *name, Main main_f, Post post_f, Logger logger)
{
    return std::make_unique<flex_driver_impl<Item, Main, Post, Logger>>(name, main_f, post_f, logger);
}


template <typename ItemIn, typename ItemOut,
	  typename Main, typename Post, typename Logger>
static inline std::unique_ptr<flex_driver<ItemIn>>
make_driver(const char *name, Main main_f, Post post_f, Logger logger)
{
    return std::make_unique<flex_driver_impl<ItemIn, ItemOut, Main, Post, Logger>>(name, main_f, post_f, logger);
}


} // namespace c7::thread::datapara


#endif // c7thread/datapara_flex.hpp
