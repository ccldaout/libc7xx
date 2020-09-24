#include <c7format.hpp>
#include <c7seq.hpp>
#include <c7utils.hpp>
#include <iostream>
#include <vector>
#include <typeinfo>
#include <ctime>

using namespace std;
#define f_ c7::format 
#define p_ c7::p_

template <typename T>
struct c7::format_traits<vector<T>> {
    static inline void convert(std::ostream& out, const std::string& format,
			       const vector<T>& vec) {
	out << "{ ";
	for (auto& v: vec) {
	    out << v << " ";
	}
	out << "}";
    }
};

enum old_enum {
    HELLO, WORLD,
};

enum class new_enum {
    HELLO, WORLD,
};

int main()
{
    p_("age: %{}, name: %{_<*} ... lang:%{}++", 10, 40, "hello world", 'C');

    const string s("Masakazu Iwanaga");
    p_("My name is %{}. weight: %{*.*f}, pointer:%{}", s, 4, 1, 63.4, (void*)&s);

    vector<int> v{ 1, 2, 3 };
    p_("vector: %{}", v);

    vector<std::string> v2{ "This", "is", "a", "pen !" };
    p_("vector<str>: %{>_30} %{#x} %{d}", v2, (short)-3276, true);

    p_("int & {}: %{:o}", 123);

    auto tv = std::time(nullptr);
    p_("tv(s) : %{T} !!", tv);
    p_("tv(us): %{t%m/%d %H:%M.%S} !!!", c7::time_us());

    old_enum oldenum = WORLD;
    p_("old enum %{}", oldenum);

    new_enum newenum = new_enum::WORLD;
    p_("new enum %{}", newenum);

    c7::com_status ios = c7::com_status::TIMEOUT;
    p_("com_status (new enum) %{}", ios);

    uint8_t u8 = 33;
    p_("<%{}> <%{d}>", u8, u8);

    p_("too few %{}_%{}_%{}_data", 1);
    p_("too many %{} data", 1, 2, 3);

    return 0;
}
