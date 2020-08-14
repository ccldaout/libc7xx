#include <c7app.hpp>
#include <c7format.hpp>

#define p_	c7::p_

int main()
{
    p_("progname: <%{}>", c7::app::progname);
    return 0;
}
