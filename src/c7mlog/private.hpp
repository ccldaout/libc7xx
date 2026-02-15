/*
 * c7mlog/private.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_MLOG_PRIVATE_HPP_LOADED_
#define C7_MLOG_PRIVATE_HPP_LOADED_


#include <cstring>
#include <optional>
#include <c7mlog.hpp>


// BEGIN: same definition with libc7/c7mlog_private.h

// _REVISION history:
//  1: initial
//  2: lnk data lnk -> lnk data thread_name lnk
//  3: thread name, source name
//  4: record max length for thread name and source name
//  5: new linkage format & enable inter process lock
//  6: lock free, no max_{tn|sn}_size
//  7: multi partition
//     8..11 skip
// 12: log_beg: uint32_t -> uint64_t
#define _REVISION		(12)

#define _IHDRSIZE		c7_align(sizeof(hdr_t), 16)
#define _DUMMY_LOG_SIZE		(16)

// rec_t internal control flags (6bits)
#define _REC_CONTROL_CHOICE	(1U << 0)

#define _TN_MAX			(63)	// tn_size:6
#define _SN_MAX			(63)	// sn_size:6

#define _PART_CNT		(C7_LOG_MAX+1)
#define _TOO_LARGE		(static_cast<raddr_t>(-1))

// END


namespace c7::mlog_impl {


using raddr_t = uint32_t;


// partition entry
struct partition7_t {
    volatile raddr_t nextaddr;
    uint32_t size_b;		// ring buffer size
};

// file header (rev1..rev6)
struct hdr6_t {
    uint32_t rev;
    volatile raddr_t nextaddr;
    volatile uint32_t cnt;
    raddr_t logsize_b;		// ring buffer size
    uint32_t hdrsize_b;		// user header size
    char hint[64];
};

// file header (rev7)
struct hdr7_t {
    uint32_t rev;
    volatile uint32_t cnt;
    uint32_t hdrsize_b;		// user header size
    uint32_t log_beg;
    char hint[64];
    partition7_t part[_PART_CNT];
};

// file header (rev12..)
struct hdr12_t {
    uint32_t rev;
    volatile uint32_t cnt;
    uint32_t hdrsize_b;		// user header size
    uint32_t _unused;
    char hint[64];
    partition7_t part[_PART_CNT];
    uint64_t log_beg;
};

// record header (rev5..)
struct rec5_t {
    raddr_t size;		// record size (rec_t + log data + names + raddr)
    uint32_t order;		// (weak) record order
    c7::usec_t time_us;		// time stamp in micro sec.
    uint64_t minidata;		// mini data (out of libc7)
    uint64_t level   :  3;	// log level 0..7
    uint64_t category:  5;	// log category 0..31
    uint64_t tn_size :  6;	// length of thread name (exclude null character)
    uint64_t sn_size :  6;	// length of source name (exclude null character)
    uint64_t src_line: 14;	// source line
    uint64_t control :  6;	// (internal control flags)
    uint64_t _rsv1   : 24;
    uint32_t pid;		// process id
    uint32_t th_id;		// thread id
    uint32_t br_order;		// ~order
};


class rbuffer7 {
private:
    partition7_t *part_ = nullptr;
    char *top_ = nullptr;
    char *end_ = nullptr;
    raddr_t size_ = 0;

public:
    rbuffer7() {}
    rbuffer7(void *headaddr, uint64_t off, partition7_t *part);

    void clear();

    raddr_t size() {
	return size_;
    }

    raddr_t nextaddr() {
	return part_->nextaddr;
    }

    raddr_t reserve(raddr_t size_b);

    void get(raddr_t addr, raddr_t size, void *_ubuf);

    raddr_t put(raddr_t addr, raddr_t size, const void *_ubuf);
};


struct rec_index_t {
    int part;
    raddr_t addr;
};

struct rec_desc_t {
    c7::usec_t time_us;
    uint32_t order;
    rec_index_t idx;
    int tn_size;
    int sn_size;
};

class rec_reader {
public:
    using info_t = mlog_reader::info_t;

    rec_reader(c7::usec_t log_beg, rbuffer7& rbuf, int part);
    std::optional<rec_desc_t> get(std::function<bool(const info_t&)>& choice,
				  raddr_t order_min,
				  c7::usec_t time_us_min,
				  std::vector<char>& dbuf);

private:
    c7::usec_t log_beg_;
    rbuffer7& rbuf_;
    int part_;
    raddr_t recaddr_;
    raddr_t brkaddr_;
};


void
make_info(mlog_reader::info_t& info, const rec5_t& rec, const char *data);

static inline bool operator<(const rec_desc_t& a, const rec_desc_t& b)
{
    return (a.time_us < b.time_us ||
	    (a.time_us == b.time_us && a.order < b.order));
}


std::unique_ptr<mlog_reader::impl> make_mlog_reader6();
std::unique_ptr<mlog_reader::impl> make_mlog_reader7();
std::unique_ptr<mlog_reader::impl> make_mlog_reader12();


} // namespace c7::mlog_impl


#endif // c7mlog_private.hpp
