// add to stream

template <typename... Args>
inline void format(std::ostream& out, const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(out);
    formatter.apply(fmts, 0, args...);
}

template <size_t N, typename... Args>
inline void format(std::ostream& out, const char (&fmt)[N], const Args&... args) noexcept
{
    format(out, analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(out, fmts, args...);
}

// add to string

template <typename... Args>
inline void format(std::string& str, const analyzed_format& fmts, const Args&... args) noexcept
{
    auto sb = c7::strmbuf::strref(str);
    auto os = std::basic_ostream(&sb);
    formatter formatter(os);
    formatter.apply(fmts, 0, args...);
}

template <size_t N, typename... Args>
inline void format(std::string& str, const char (&fmt)[N], const Args&... args) noexcept
{
    format(str, analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void format(std::string& str, const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(str, fmts, args...);
}

// new string

template <typename... Args>
inline std::string format(const analyzed_format& fmts, const Args&... args) noexcept
{
    std::string str;
    format(str, fmts, args...);
    return str;
}

template <size_t N, typename... Args>
inline std::string format(const char (&fmt)[N], const Args&... args) noexcept
{
    std::string str;
    format(str, analyzed_format::get_for_literal(&fmt[0]), args...);
    return str;
}

template <typename... Args>
inline std::string format(const std::string& fmt, const Args&... args) noexcept
{
    std::string str;
    format(str, fmt, args...);
    return str;
}

// std::cout without NL(new line) and with lock

template <typename... Args>
inline void P_(const analyzed_format& fmts, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <size_t N, typename... Args>
inline void P_(const char (&fmt)[N], const Args&... args) noexcept
{
    P_(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void P_(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_(fmts, args...);
}

// std::cout without NL(new line) and with lock

template <typename... Args>
inline void P_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <size_t N, typename... Args>
inline void P_nolock(const char (&fmt)[N], const Args&... args) noexcept
{
    P_nolock(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void P_nolock(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_nolock(fmts, args...);
}

// std::cout with NL(new line) and with lock

template <typename... Args>
inline void p_(const analyzed_format& fmts, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
    std::cout << std::endl;
}

template <size_t N, typename... Args>
inline void p_(const char (&fmt)[N], const Args&... args) noexcept
{
    p_(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_(fmts, args...);
}

// std::cout with NL(new line) and without lock

template <typename... Args>
inline void p_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
    std::cout << std::endl;
}

template <size_t N, typename... Args>
inline void p_nolock(const char (&fmt)[N], const Args&... args) noexcept
{
    p_nolock(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void p_nolock(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_nolock(fmts, args...);
}
