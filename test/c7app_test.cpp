#include <c7app.hpp>
#include <c7format.hpp>

#define p_	c7::p_

int main(int argc, char **argv)
{
    p_("progname: <%{}>", c7::app::progname);
    p_("-");
    c7echo("simple echo");
    p_("-");
    c7echo("errno: %{}", errno);
    p_("-");
    c7echo(c7result_err(EINVAL, "result_err"), "with res");
    p_("-");
    c7echo(c7result_err(EINVAL, "result_err"), "format with res: %{}", *argv);
    p_("-");
    if (argc == 1) {
	c7exit(c7result_err(EINVAL, "result_err"), "EXIt: format with res: %{}", *argv);
    } else if (argc == 2) {
	c7error(c7result_err(EINVAL, "result_err"), "ERROR: format with res: %{}", *argv);
    } else {
	c7abort(c7result_err(EINVAL, "result_err"), "ABORT: format with res: %{}", *argv);
    }
    return 0;
}
