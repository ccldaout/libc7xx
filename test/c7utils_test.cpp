#include <c7format.hpp>
#include <c7utils.hpp>


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


struct data_t {
    int data[10];
    typedef c7::c_array_iterator<int> iterator;
    typedef c7::c_array_iterator<const int> const_iterator;

    data_t() {
	for (int i = 0; i < c7_numberof(data); i++) {
	    data[i] = i;
	}
    }

    iterator begin() {
	return iterator(data, 0);
    }
    iterator end() {
	return iterator(data, c7_numberof(data));
    }
    const_iterator begin() const {
	p_("const_iterator");
	return const_iterator(data, 0);
    }
    const_iterator end() const {
	return const_iterator(data, c7_numberof(data));
    }
};

static void c_array_test()
{
    data_t data;
    for (auto& d: data) {
	p_("%{} ", d);
    }
    p_("");
    
    p_("--");

    const data_t& cdata = data;
    for (auto& d: cdata) {
	p_("%{} ", d);
    }
    p_("");
}


int main()
{
    pwd_test();
    c_array_test();
    return 0;
}
