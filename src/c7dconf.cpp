/*
 * c7dconf.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7dconf.hpp>
#include <c7nseq/enumerate.hpp>
#include <c7path.hpp>
#include <cstring>
#include <stdexcept>


namespace c7 {


static std::vector<dconf_def> sys_defv{
    C7_DCONF_DEF_I3(C7_DCONF_rsv_91, MLOG_former, "--- (moved to MLOG_MAIN) ---"),
    C7_DCONF_DEF_I3(C7_DCONF_MLOG, MLOG_MAIN, "mlog level (default:4)"),
    C7_DCONF_DEF_I3(C7_DCONF_MLOG_LIBC7, MLOG_LIBC7, "mlog level: libc7++"),
};


static std::string dconfpath(const std::string& path, bool exists)
{
    if (exists) {
	return c7::path::find_c7spec(path, ".dconf", C7_DCONF_DIR_ENV);
    } else {
	return c7::path::init_c7spec(path, ".dconf", C7_DCONF_DIR_ENV);
    }
}


// on producer side

static size_t sizeofhelp(const std::vector<dconf_def>& defv)
{
    size_t n = 0;
    for (auto& d: defv) {
	n += d.ident.size() + d.descrip.size() + 1;	// + 1: ':'
    }
    n += C7_DCONF_INDEX_LIM * 1;	 		// all index has '\a'.
    n++;						// last null
    return n;
}

static const dconf_def *finddef(const std::vector<dconf_def>& defv, int index)
{
    for (auto& d: defv) {
	if (d.index == index) {
	    return &d;
	}
    }
    return nullptr;
}

static bool check_user_index(int index)
{
    return ((C7_DCONF_USER_INDEX_BASE <= index && index < C7_DCONF_USER_INDEX_LIM) ||
	    (C7_DCONF_MLOG_1 <= index && index < C7_DCONF_MLOG_LIBC7));
}

static std::vector<dconf_def> mergedef(const std::vector<dconf_def>& user_defv)
{
    auto defv = sys_defv;
    for (auto [i, d]: user_defv | c7::nseq::enumerate()) {
	if (!check_user_index(d.index)) {
	    throw std::runtime_error(
		c7::format("%{}th dconf index:%{} is out of range", i, d.index));
	}
	defv.push_back(d);
    }

    return defv;
}

static c7::file::unique_mmap<c7_dconf_head_t>
mapalt(size_t n)
{
    auto p = mmap(nullptr, n, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
	throw std::runtime_error("cannot allocate dconf storage");
    }
    return c7::file::unique_mmap<c7_dconf_head_t>(reinterpret_cast<c7_dconf_head_t*>(p),
						  c7::file::mmap_deleter(n));
}

static c7::file::unique_mmap<c7_dconf_head_t>
mapdconf(const std::string& name, const std::vector<dconf_def>& defv)
{
    auto path = dconfpath(name, false);

    size_t n = sizeof(c7_dconf_head_t) + sizeofhelp(defv);

    auto res = c7::file::mmap_rw<c7_dconf_head_t>(path, n, true);
    if (!res) {
	res = c7result_ok(mapalt(n));
    }
    auto dh = res.value().get();

    if (dh->version < C7_DCONF_VERSION) {
	for (int i = C7_DCONF_MLOG_BASE; i < C7_DCONF_INDEX_LIM; i++) {
	    dh->array[i].i = 0;
	}
	if (dh->version <= 2) {
	    dh->array[C7_DCONF_MLOG].i = dh->array[C7_DCONF_rsv_91].i;
	}
    }

    dh->version = C7_DCONF_VERSION;
    dh->mapsize = n;

    // Memory area next c7_dconf_head_t hold some helpful text for c7dconf command.

    char *p = (char *)(dh + 1);
    for (int i = 0; i < C7_DCONF_INDEX_LIM; i++) {
	auto def = finddef(defv, i);
	if (def != nullptr) {
	    p = c7::strcpy(p, def->ident.c_str());
	    *p++ = ':';
	    p = c7::strcpy(p, def->descrip.c_str());
	    *p++ = '\a';
	} else {
	    *p++ = '\a';
	}
    }
    *p = 0;

    return std::move(res.value());
}

void dconf_type::add_defs(const std::vector<dconf_def>& defv)
{
    for (auto& d: defv) {
	if (finddef(sys_defv, d.index) == nullptr) {
	    sys_defv.push_back(d);
	}
    }
}

void dconf_type::init(const std::string& name, const std::vector<dconf_def>& user_defv)
{
    auto defv = mergedef(user_defv);
    storage_ = mapdconf(name, defv);

    int level = C7_LOG_BRF;

    auto& self = *this;
    if (self[C7_DCONF_MLOG].i == 0) {
	if (0 < self[C7_DCONF_rsv_91].i && self[C7_DCONF_rsv_91].i < 8) {
	    level = self[C7_DCONF_rsv_91].i;
	}
	self[C7_DCONF_MLOG].i = level;
    }
    self[C7_DCONF_rsv_91].i = self[C7_DCONF_MLOG].i;

    for (auto& d: defv) {
	storage_->types[d.index] = d.type;
    }
}


// on consumer side (c7dconf command)

static c7::result<c7::file::unique_mmap<c7_dconf_head_t>>
loaddconf(const std::string& name)
{
    auto path = dconfpath(name, true);

    size_t n = 0;
    auto res = c7::file::mmap_rw<c7_dconf_head_t>(path, n, false);
    if (!res) {
	return res;
    }

    auto dh = res.value().get();
    if (dh->version != C7_DCONF_VERSION) {
	return c7result_err("version mismatch: file:%{}, lib:%{}",
			    dh->version, C7_DCONF_VERSION);
    }
    return res;
}

static std::vector<dconf_def> makedef(const c7_dconf_head_t *dh)
{
    std::vector<dconf_def> defv;
    dconf_def def;

    char *p = (char *)(dh + 1);
    for (int index = 0; *p != 0; p++, index++) {
	if (*p == '\a') {
	    continue;
	}
	def.index = index;
	def.type = dh->types[index];

	auto q = std::strchr(p, ':');
	def.ident = std::string(p, q - p);

	p = q + 1;
	q = std::strchr(p, '\a');
	def.descrip = std::string(p, q - p);

	defv.push_back(def);

	p = q;
    }

    return defv;
}

c7::result<std::vector<dconf_def>> dconf_type::load(const std::string& name)
{
    auto res = loaddconf(name);
    if (!res) {
	return res.as_error();
    }
    storage_ = std::move(res.value());
    return c7result_ok(makedef(storage_.get()));
}


// default

static int get_i(const std::string &env, int defval)
{
    auto v = ::getenv(env.c_str());
    if (v == nullptr) {
	return defval;
    }
    return std::strtol(v, nullptr, 0);
}

dconf_type::dconf_type()
{
    storage_ = mapalt(sizeof(c7_dconf_head_t));
    auto& self = *this;
    self[C7_DCONF_MLOG].i         = get_i("C7_DCONF_MLOG", C7_LOG_BRF);
}


dconf_type dconf;


} // namespace c7
