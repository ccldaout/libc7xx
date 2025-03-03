/*
 * c7args.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_ARGS_HPP_LOADED_
#define C7_ARGS_HPP_LOADED_
#include <c7common.hpp>


#include <regex>
#include <c7string.hpp>


namespace c7::args {


struct opt_desc {
    enum class prm_type {
	BOOL,
	INT,
	UINT,
	REAL,
	KEY,		// keyword list
	REG,		// regular expression list
	DATE,		// date and time: [[YY]MMDD]hhmm[.ss] or .
	ANY,
	numof,
    };

    // option features
    // - option name
    std::string long_name;		// exclude leading '--'
    std::string short_name;		// empty or just one character
    // - parameter features
    prm_type type = prm_type::BOOL;	// parameter type
    unsigned int prmc_min = 0;		// parameter count limitation: lower
    unsigned int prmc_max = 0;		// parameter count limitation: upper
    c7::strvec keys;			// [type:KEY,REG] keyword or regex-pattern list
    bool ignore_case = false;		// [type:KEY,REG]

    // for usage or message
    std::string opt_descrip;
    std::string prm_name;
    std::string prm_descrip;
    c7::strvec  reg_descrips;		// [type:REG] human readable explanations
};


struct opt_value {
    using prm_type = opt_desc::prm_type;
    prm_type type;
    std::string param;
    c7::strvec regmatch;		// [type:REG, type:DATE]
    int key_index = -1;			// [type:KEY, type:REG]
    union {
	bool b = false;			// [type:BOOL]
	int64_t i;			// [type:INT]
	uint64_t u;			// [type:UINT]
	time_t t;			// [type:DATE]
	double d;			// [type:REAL]
    };
};


class parser {
public:
    using opt_desc	= c7::args::opt_desc;
    using opt_value	= c7::args::opt_value;
    using prm_type	= opt_desc::prm_type;
    using callback_t	= c7::result<>(const opt_desc&,
				       const std::vector<opt_value>&);
    using callback_obj	= std::function<callback_t>;

    parser();
    virtual ~parser();

    virtual c7::result<> init() = 0;
    c7::result<char**> parse(char **argv);
    void append_usage(c7::strvec& usage, size_t base_indent, size_t descrip_indent) const;

protected:
    // add_opt should be called in init() override.
    template <typename Derived>
    c7::result<> add_opt(const opt_desc& desc,
			 c7::result<>(Derived::*mfp)(const opt_desc&,
						     const std::vector<opt_value>&)) {
	return add_opt(desc,
		       [this, mfp](const opt_desc& d,
				   const std::vector<opt_value>& v) {
			   return (static_cast<Derived*>(this)->*mfp)(d, v);
		       });
    }

    c7::result<> add_opt(const opt_desc&, callback_obj);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


void print_type(std::ostream&, const std::string&, opt_desc::prm_type);
void print_type(std::ostream&, const std::string&, const opt_desc&);
void print_type(std::ostream&, const std::string&, const opt_value&);


} // namespace c7::args


#endif // c7args.hpp
