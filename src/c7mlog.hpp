/*
 * c7mlog.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=2145689840
 */
#ifndef C7_MLOG_HPP_LOADED_
#define C7_MLOG_HPP_LOADED_
#include <c7common.hpp>


#include <c7format.hpp>
#include <c7format/format_r2.hpp>
#include <c7format/format_r3.hpp>
#include <c7result.hpp>
#include <c7strmbuf/hybrid.hpp>
//#include <c7thread.hpp>
#include <c7utils.hpp>
#include <string>


// API support flags

#define C7_MLOG_API_set_callback	(1U<<0)		// set_callback, enable_stdout


// BEGIN: same definition with c7mlog.[ch]

#define C7_MLOG_DIR_ENV		"C7_MLOG_DIR"

#define C7_MLOG_SIZE_MIN	(1024)
#define C7_MLOG_SIZE_MAX	(1U << 30)

// log level: 0..7 (cf. C7_LOG_xx defined at c7commons.hpp)

// log category: 0..31
#define C7_MLOG_C_MIN		(0U)
#define C7_MLOG_C_MAX		(31U)

// w_flags
#define C7_MLOG_F_THREAD_NAME	(1U << 0)	// record thread name
#define C7_MLOG_F_SOURCE_NAME	(1U << 1)	// record source name
#define C7_MLOG_F_SPINLOCK	(1U << 2)	// (DON'T EFFECT)

// END


namespace c7 {


class mlog_writer {
private:
    class impl;
    impl *pimpl;

public:
    using callback_t =
	std::function<void(c7::usec_t time_us, const char *src_name, int src_line,
			   uint32_t level, uint32_t category, uint64_t minidata,
			   const void *logaddr, size_t logsize_b)>;

    mlog_writer(const mlog_writer&) = delete;
    mlog_writer& operator=(const mlog_writer&) = delete;

    mlog_writer();
    ~mlog_writer();

    mlog_writer(mlog_writer&& o);
    mlog_writer& operator=(mlog_writer&& o);

    result<> init(const std::string& name,
		      size_t hdrsize_b,
		      size_t logsize_b,
		      uint32_t w_flags,
		      const char *hint = nullptr);

    void set_callback(callback_t);

    void enable_stdout();	// set_callback with internal print function

    void *hdraddr(size_t *hdrsize_b_op = nullptr);

    void post_forked();

    bool put(c7::usec_t time_us, const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const void *logaddr, size_t logsize_b);

    bool put(const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const void *logaddr, size_t logsize_b);

    bool put(const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const std::string& s);

    bool put(const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const char *s);

    // default c7::format

    template <typename... Args>
    void format(c7::usec_t time_us, const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format(c7::usec_t time_us, const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format(const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format(const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    // c7::format_r2

    template <typename... Args>
    void format(c7::usec_t time_us, const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const c7::format_r2::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r2(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const c7::format_r2::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r2(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r2(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format(const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const c7::format_r2::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r2(const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r2(const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r2::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    // c7::format_r3

    template <typename... Args>
    void format(c7::usec_t time_us, const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const c7::format_r3::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const c7::format_r3::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3_var(c7::usec_t time_us, const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format_var(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3(c7::usec_t time_us, const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format(const char *src_name, int src_line,
		uint32_t level, uint32_t category, uint64_t minidata,
		const c7::format_r3::analyzed_format& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3(const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3_var(const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const char *format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format_var(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    template <typename... Args>
    void format_r3(const char *src_name, int src_line,
		   uint32_t level, uint32_t category, uint64_t minidata,
		   const std::string& format, const Args&... args) {
	auto sb = c7::strmbuf::hybrid();
	auto os = std::basic_ostream(&sb);
	c7::format_r3::format(os, format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  sb.data(), sb.size() + 1);
    }

    // error

    void error(c7::usec_t time_us, const char *src_name, int src_line,
	       uint32_t level, uint32_t category, uint64_t minidata,
	       const c7::result_base& res) {
	std::stringstream ss;
	c7::format_traits<result_base>::convert(ss, "", res);
	auto s = ss.str();
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  s.c_str(), s.size() + 1);
    }

    void error(const char *src_name, int src_line,
	       uint32_t level, uint32_t category, uint64_t minidata,
	       const c7::result_base& res) {
	(void)error(c7::time_us(), src_name, src_line, level, category, minidata, res);
    }
};


class mlog_reader {
public:
    struct info_t {
	const char *thread_name;
	uint64_t thread_id;
	const char *source_name;
	int source_line;
	uint32_t weak_order;	// record serial number (NOT STRICT)
	int32_t size_b;		// record size
	c7::usec_t time_us;	// time stamp in micro sec.
	uint32_t level;
	uint32_t category;
	uint64_t minidata;
	pid_t pid;
    };

private:
    class impl;
    impl *pimpl;

public:
    mlog_reader(const mlog_reader&) = delete;
    mlog_reader& operator=(const mlog_reader&) = delete;

    mlog_reader();
    ~mlog_reader();

    mlog_reader(mlog_reader&& o);
    mlog_reader& operator=(mlog_reader&& o);

    result<> load(const std::string& name);

    void scan(size_t maxcount,
	      uint32_t order_min,
	      c7::usec_t time_us_min,
	      std::function<bool(const info_t& info)> choice,
	      std::function<bool(const info_t& info, void *data)> access);

    void *hdraddr();
    int thread_name_size();
    int source_name_size();
    const char *hint();
};


extern mlog_writer mlog;


} // namespace c7


#endif // c7mlog.hpp
