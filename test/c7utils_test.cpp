#include "c7format.hpp"
#include "c7utils.hpp"


using namespace std;

#define p_	c7::p_


static void pwd_test()
{
    std::vector<std::string> users{"ebsys", "noname"};
    for (auto user: users) {
	auto r = c7::passwd::by_name(user);
	if (r) {
	    auto& pwd = r.value();
	    p_("%{} -> %{}, %{}, %{}", user, pwd->pw_uid, pwd->pw_dir, pwd->pw_shell);
	} else {
	    p_("by_name error: %{}", r);
	}
    }

    std::vector<::uid_t> uids{1204, 1200};
    for (auto uid: uids) {
	auto r = c7::passwd::by_uid(uid);
	if (r) {
	    auto& pwd = r.value();
	    p_("%{} -> %{}, %{}, %{}", uid, pwd->pw_name, pwd->pw_dir, pwd->pw_shell);
	} else {
	    p_("by_uid error: %{}", r);
	}
    }	
}


int main()
{
    pwd_test();

    return 0;
}
