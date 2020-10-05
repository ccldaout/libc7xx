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
#ifndef C7_MLOG_HPP_LOADED__
#define C7_MLOG_HPP_LOADED__
#include <c7common.hpp>


#include <c7format.hpp>
#include <c7result.hpp>
#include <c7thread.hpp>
#include <c7utils.hpp>
#include <string>


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
#define C7_MLOG_F_SPINLOCK	(1U << 2)	// use spinlock

// END


namespace c7 {


class mlog_writer {
private:
    class impl;
    impl *pimpl;

public:
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

    void *hdraddr(size_t *hdrsize_b_op = nullptr);
    
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

    template <typename... Args>
    inline void format(c7::usec_t time_us, const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const std::string& format, const Args&... args) {
	auto s = c7::format(format, args...);
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  s.c_str(), s.size() + 1);
    }

    template <typename... Args>
    inline void format(const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const std::string& format, const Args&... args) {
	auto s = c7::format(format, args...);
	(void)put(c7::time_us(), src_name, src_line, level, category, minidata,
		  s.c_str(), s.size() + 1);
    }

    inline void error(c7::usec_t time_us, const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const c7::result_base& res) {
	std::stringstream ss;
	c7::format_traits<result_base>::convert(ss, "", res);
	auto s = ss.str();
	(void)put(time_us, src_name, src_line, level, category, minidata,
		  s.c_str(), s.size() + 1);
    }

    inline void error(const char *src_name, int src_line,
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
	uint32_t order;		// record serial number
	int32_t size_b;		// record size
	c7::usec_t time_us;	// time stamp in micro sec.
	uint32_t level;
	uint32_t category;
	uint64_t minidata;
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


} // namespace c7


#endif // c7mlog.hpp
