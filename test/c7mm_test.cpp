#include <c7format.hpp>
#include <c7mm.hpp>
#include <c7path.hpp>
#include <vector>

#define p_	c7::p_

struct data_t {
    size_t v;
    char s[1024-sizeof(size_t)];
};

#define INI	( 1L *1024*1024)
#define THR	(32L *1024*1024)

template <typename C>
static void tail(const C& mmv)
{
    p_("-- reverse->head");
    for (auto& v: c7::seq::head(c7::seq::reverse(mmv), 5)) {
	p_("%{}", v.v);
    }
}

int main()
{
#if 1
    auto path = c7::path::untildize("~/tmp/mmvec.tset.mmap");
    c7::mmvec<data_t> mmv;
    if (auto res = mmv.init(INI, THR, path); !res) {
	p_("mmv.init %{}", res);
	return 1;
    }
#else
    std::vector<data_t> mmv;
#endif
    int n = 4*1024*1024;
    for (int i = 0; i < n; i++) {
	data_t d;
	d.v = i;
	mmv.push_back(d);
    }
    p_("--");
    for (auto& v: c7::seq::head(mmv, 5)) {
	p_("%{}", v.v);
    }
    tail(mmv);

    return 0;
}

