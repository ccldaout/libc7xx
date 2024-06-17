/*
 * c7dconf.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1334956066
 */
#ifndef C7_DCONF_HPP_LOADED_
#define C7_DCONF_HPP_LOADED_
#include <c7common.hpp>


#include <c7file.hpp>
#include <string>
#include <vector>


// BEGIN: same definition with c7dconf.[ch]

#define C7_DCONF_DIR_ENV		"C7_DCONF_DIR"
#define C7_DCONF_USER_INDEX_BASE	(0)
#define C7_DCONF_USER_INDEX_LIM		(90)	// not include 90
#define C7_DCONF_MLOG_BASE		(100)
#define C7_DCONF_INDEX_LIM		(150)	// C7_DCONF_USER_INDEX_LIM..149: libc7 area
#define C7_DCONF_VERSION		(3)	// C7_INDEX_LIM:150

enum c7_dconf_reserve_t {
    C7_DCONF_rsv_90 = C7_DCONF_USER_INDEX_LIM,
    C7_DCONF_rsv_91,		// former MLOG level
    C7_DCONF_rsv_92,
    C7_DCONF_rsv_93,
    C7_DCONF_rsv_94,
    // all 32 indexes between C7_DCONF_MLOG and C7_DCONF_MLOG_LIBC7 are for mlog
    C7_DCONF_MLOG = C7_DCONF_MLOG_BASE,
    C7_DCONF_MLOG_1,
    C7_DCONF_MLOG_2,
    C7_DCONF_MLOG_3,
    C7_DCONF_MLOG_4,
    C7_DCONF_MLOG_5,
    C7_DCONF_MLOG_6,
    C7_DCONF_MLOG_7,
    C7_DCONF_MLOG_8,
    C7_DCONF_MLOG_9,
    C7_DCONF_MLOG_10,
    C7_DCONF_MLOG_11,
    C7_DCONF_MLOG_12,
    C7_DCONF_MLOG_13,
    C7_DCONF_MLOG_14,
    C7_DCONF_MLOG_15,
    C7_DCONF_MLOG_16,
    C7_DCONF_MLOG_17,
    C7_DCONF_MLOG_18,
    C7_DCONF_MLOG_19,
    C7_DCONF_MLOG_20,
    C7_DCONF_MLOG_21,
    C7_DCONF_MLOG_22,
    C7_DCONF_MLOG_23,
    C7_DCONF_MLOG_24,
    C7_DCONF_MLOG_25,
    C7_DCONF_MLOG_26,
    C7_DCONF_MLOG_27,
    C7_DCONF_MLOG_28,
    C7_DCONF_MLOG_29,
    C7_DCONF_MLOG_30,
    C7_DCONF_MLOG_LIBC7 = C7_DCONF_MLOG + 31,
    C7_DCONF_numof
};

enum c7_dconf_type_t {
    C7_DCONF_TYPE_None,
    C7_DCONF_TYPE_I64,
    C7_DCONF_TYPE_R64
};

union c7_dconf_val_t {
    int64_t i;
    double r;
};

struct c7_dconf_head_t {
    int32_t version;
    uint32_t mapsize;
    c7_dconf_val_t array[C7_DCONF_INDEX_LIM];
    c7_dconf_type_t types[C7_DCONF_INDEX_LIM];
};

// END


namespace c7 {


struct dconf_def {
    int index;
    c7_dconf_type_t type;
    std::string ident;
    std::string descrip;
};

#define C7_DCONF_DEF_I(idxmacro, descrip)			\
    { (idxmacro), C7_DCONF_TYPE_I64, #idxmacro, descrip }

#define C7_DCONF_DEF_I3(idxmacro, idxname, descrip)		\
    { (idxmacro), C7_DCONF_TYPE_I64, #idxname, descrip }

#define C7_DCONF_DEF_R(idxmacro, descrip)			\
    { (idxmacro), C7_DCONF_TYPE_R64, #idxmacro, descrip }

#define C7_DCONF_DEF_R3(idxmacro, idxname, descrip)		\
    { (idxmacro), C7_DCONF_TYPE_R64, #idxname, descrip }


class dconf_type {
private:
    c7::file::unique_mmap<c7_dconf_head_t> storage_;

public:
    dconf_type();

    // on consumer side
    c7::result<std::vector<dconf_def>> load(const std::string& name);

    // on producer side
    void add_defs(const std::vector<dconf_def>& defv);
    void init(const std::string& name, const std::vector<dconf_def>& defv);

    dconf_type(const std::string& name, const std::vector<dconf_def>& defv) {
	init(name, defv);
    }

    // on both side
    c7_dconf_val_t& operator[](int index) {
	return storage_->array[index];
    }

    const c7_dconf_val_t& operator[](int index) const {
	return storage_->array[index];
    }
};


extern dconf_type dconf;


} // namespace c7


#endif // c7dconf.hpp
