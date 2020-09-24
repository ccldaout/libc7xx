#include <c7dconf.hpp>
#include <c7format.hpp>

#define p_ 	c7::p_

enum {
    CONF_1,
    CONF_2,
    CONF_3 = 13,
    CONF_4,
};

static std::vector<int> idx{
    CONF_1,
    CONF_2,
    CONF_3,
    CONF_4,
};

static std::vector<c7::dconf_def> defs{
    C7_DCONF_DEF_I(CONF_1, "configuration 1"),
    C7_DCONF_DEF_I(CONF_2, "configuration 2"),
    C7_DCONF_DEF_I(CONF_3, "configuration III1"),
    C7_DCONF_DEF_I(CONF_4, "configuration four"),
};

static c7::dconf myconf("myconf", defs);

int main()
{
    for (auto i: idx) {
	p_("myconf %{} : %{}", i, myconf[i].i);
	myconf[i].i += 100;
    }
    
    c7::dconf rdconf;
    auto res = rdconf.load("myconf");
    if (!res)
	p_("%{}", res);
    else {
	auto dv = res.value();
	for (auto& d: dv) {
	    p_("#%{} type:%{} ident:'%{}' desc:'%{}' : %{}",
	       d.index, d.type, d.ident, d.descrip, rdconf[d.index].i);
	}
   }

    return 0;
}
