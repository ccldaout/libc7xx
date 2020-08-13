/*
 * c7dconf.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_DCONF_HPP_LOADED__
#define __C7_DCONF_HPP_LOADED__
#include "c7common.hpp"


#include "c7file.hpp"
#include <string>
#include <vector>


// BEGIN: same definition with c7dconf.[ch]

#define C7_DCONF_DIR_ENV		"C7_DCONF_DIR"
#define C7_DCONF_USER_INDEX_BASE	(0)
#define C7_DCONF_USER_INDEX_LIM		(90)	// not include 90
#define C7_DCONF_INDEX_LIM		(100)	// C7_DCONF_USER_INDEX_LIM..99: libc7 area
#define C7_DCONF_VERSION		(2)	// C7_INDEX_LIM:100, C7_DCONF_USER_INDEX_LIM:90

enum c7_dconf_reserve_t {
    C7_DCONF_ECHO = C7_DCONF_USER_INDEX_LIM,
    C7_DCONF_MLOG,
    C7_DCONF_PREF,
    C7_DCONF_STSSCN_MAX,
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

#define C7_DCONF_DEF_R(idxmacro, descrip)			\
    { (idxmacro), C7_DCONF_TYPE_R64, #idxmacro, descrip }
    

class dconf {
private:
    c7::file::unique_mmap<c7_dconf_head_t> storage_;

public:
    dconf();

    // on consumer side
    c7::result<std::vector<dconf_def>> load(const std::string& name);

    // on producer side
    void init(const std::string& name, const std::vector<dconf_def>& defv);

    dconf(const std::string& name, const std::vector<dconf_def>& defv) {
	init(name, defv);
    }

    // on both side
    inline c7_dconf_val_t& operator[](int index) {
	return storage_->array[index];
    }
};


} // namespace c7


#endif // c7dconf.hpp
