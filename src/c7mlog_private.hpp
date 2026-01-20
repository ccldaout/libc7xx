/*
 * c7mlog_private.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_MLOG_PRIVATE_HPP_LOADED_
#define C7_MLOG_PRIVATE_HPP_LOADED_


#include <cstring>
#include <c7mlog.hpp>


// BEGIN: same definition with libc7/c7mlog_private.h

// _REVISION history:
// 1: initial
// 2: lnk data lnk -> lnk data thread_name lnk
// 3: thread name, source name
// 4: record max length for thread name and source name
// 5: new linkage format & enable inter process lock
// 6: lock free, no max_{tn|sn}_size
// 7: multi partition
#define _REVISION		(7)

#define _IHDRSIZE		c7_align(sizeof(hdr_t), 16)
#define _DUMMY_LOG_SIZE		(16)

// rec_t internal control flags (6bits)
#define _REC_CONTROL_CHOICE	(1U << 0)

#define _TN_MAX			(63)	// tn_size:6
#define _SN_MAX			(63)	// sn_size:6

#define _PART_CNT		(C7_LOG_MAX+1)
#define _TOO_LARGE		(static_cast<raddr_t>(-1))

// END


namespace c7 {


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

// file header (rev7..)
struct hdr7_t {
    uint32_t rev;
    volatile uint32_t cnt;
    uint32_t hdrsize_b;		// user header size
    uint32_t log_beg;
    char hint[64];
    partition7_t part[_PART_CNT];
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

    rbuffer7(void *headaddr, uint64_t off, partition7_t *part):
	part_(part),
	top_(static_cast<char*>(headaddr) + off),
	end_(top_ + part->size_b),
	size_(part->size_b) {
    }

    void clear() {
	if (size_ > 0) {
	    raddr_t addr = 0;
	    part_->nextaddr = put(addr, sizeof(addr), &addr);
	}
    }

    raddr_t size() {
	return size_;
    }

    raddr_t nextaddr() {
	return part_->nextaddr;
    }

    raddr_t reserve(raddr_t size_b) {
	// check size to be written
	if ((size_b + 32) > size_) {	// (C) ensure rechdr.size < hdr_->logsize_b
	    return _TOO_LARGE;		// data size too large
	}

	volatile raddr_t * const nextaddr_p = &part_->nextaddr;
	raddr_t addr, next;
	do {
	    addr = *nextaddr_p;
	    next = (addr + size_b) % size_;
	    // addr != next is ensured by above check (C)
	} while (__sync_val_compare_and_swap(nextaddr_p, addr, next) != addr);
	return addr;
    }

    void get(raddr_t addr, raddr_t size, void *_ubuf) {
	char *ubuf = static_cast<char*>(_ubuf);
	char *rbuf = top_ + (addr % size_);
	raddr_t rrest = end_ - rbuf;

	while (size > 0) {
	    raddr_t cpsize = std::min(size, rrest);

	    std::memcpy(ubuf, rbuf, cpsize);
	    ubuf += cpsize;
	    size -= cpsize;

	    rbuf = top_;
	    rrest = size_;
	}
    }

    raddr_t put(raddr_t addr, raddr_t size, const void *_ubuf) {
	const raddr_t ret_addr = addr + size;

	const char *ubuf = static_cast<const char*>(_ubuf);
	char *rbuf = top_ + (addr % size_);
	raddr_t rrest = end_ - rbuf;

	while (size > 0) {
	    raddr_t cpsize = (size < rrest) ? size : rrest;

	    std::memcpy(rbuf, ubuf, cpsize);
	    ubuf += cpsize;
	    size -= cpsize;

	    rbuf = top_;
	    rrest = size_;
	}

	return ret_addr;
    }
};


std::unique_ptr<mlog_reader::impl> make_mlog_reader6();


} // namespace c7


#endif // c7mlog_private.hpp
