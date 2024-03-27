/*
 * c7nseq/command.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_COMMAND_HPP_LOADED_
#define C7_NSEQ_COMMAND_HPP_LOADED_


#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <vector>
#include <c7pipeline.hpp>
#include <c7socket.hpp>
#include <c7string.hpp>
#include <c7thread.hpp>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/empty.hpp>


namespace c7::nseq {


struct command_iter_end {};


template <typename Output>
class command_iter {
public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= c7::typefunc::remove_cref_t<Output>;
    using pointer		= value_type*;
    using reference		= value_type&;

private:
    c7::pipeline *pl_;
    c7::fd *rfd_;
    size_t buffer_size_;
    std::vector<Output> vec_;
    size_t idx_ = 0;

    void readvec() {
	vec_.resize(buffer_size_);
	auto iores = rfd_->read_n(static_cast<void*>(vec_.data()),
				sizeof(Output) * vec_.capacity());
	if (iores.get_status() == c7::io_result::status::OK ||
	    iores.get_status() == c7::io_result::status::INCOMP) {
	    vec_.resize(iores.get_done()/sizeof(Output));
	    idx_ = 0;
	} else {
	    rfd_->close();
	}
    }

public:
    command_iter() = default;
    command_iter(const command_iter&) = default;
    command_iter& operator=(const command_iter&) = default;
    command_iter(command_iter&&) = default;
    command_iter& operator=(command_iter&&) = default;

    command_iter(c7::pipeline& pl, c7::fd& rfd, size_t buffer_size):
	pl_(&pl), rfd_(&rfd), buffer_size_(buffer_size) {
	vec_.reserve(buffer_size);
	vec_.resize(buffer_size);
	readvec();
    }

    bool operator==(const command_iter& o) const {
	return rfd_ == o.rfd_;
    }

    bool operator!=(const command_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const command_iter_end&) const {
	if (rfd_ == nullptr || pl_ == nullptr) {
	    return true;
	}
	if (*rfd_) {
	    return false;
	}
	for (auto s: pl_->wait()) {
	    if (s != 0 && s != -SIGPIPE) {
		auto err = s > 0 ? s : EFAULT;
		c7result_err(err, "pipeline failed").raise();
	    }
	}
	return true;
    }

    bool operator!=(const command_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (idx_ == vec_.size() - 1) {
	    readvec();
	} else {
	    idx_++;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return std::move(vec_[idx_]);
    }
};


template <typename Seq, typename Output>
class command_ctx {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    c7::pipeline pl_;
    c7::fd rfd_;
    size_t buffer_size_;

    c7::result<> sender_thread(int fd) {
	c7::fd wfd{fd};
	std::vector<Output> buf;
	buf.reserve(buffer_size_);
	for (auto&& v: seq_) {
	    if (buf.size() == buf.capacity()) {
		auto iores = wfd.write_n(static_cast<const void *>(buf.data()),
					 sizeof(Output) * buf.size());
		if (!iores) {
		    return std::move(iores.get_result());
		}
		buf.clear();
	    }
	    buf.push_back(std::forward<decltype(v)>(v));
	}
	if (buf.size() > 0) {
	    auto iores = wfd.write_n(static_cast<const void *>(buf.data()),
				     sizeof(Output) * buf.size());
	    if (!iores) {
		return std::move(iores.get_result());
	    }
	}
	return c7result_ok();
    }

public:
    command_ctx(Seq seq, c7::pipeline&& pl, size_t buffer_size):
	seq_(std::forward<Seq>(seq)), pl_(std::move(pl)), buffer_size_(buffer_size) {
	int pfd[2];
	pl_.exec(pfd).check();
	rfd_ = c7::fd{pfd[0]};
	c7::thread::thread th;
	th.target([this, fd=pfd[1]](){
		      sender_thread(fd).check();
		  });
	th.set_name("pipeline[s]");
	th.start().check();
    }

    ~command_ctx() {
	pl_.kill(9);
	pl_.wait();
    }

    auto begin() {
	return command_iter<Output>(pl_, rfd_, buffer_size_);
    }

    auto end() {
	return command_iter_end{};
    }

    auto begin() const {
	return const_cast<command_ctx<Seq, Output>*>(this)->begin();
    }

    auto end() const {
	return command_iter_end{};
    }
};


template <typename Seq, typename Output>
class command_seq {
private:
    std::shared_ptr<command_ctx<Seq, Output>> ctx_;

public:
    command_seq(Seq seq, c7::pipeline&& pl, size_t buffer_size):
	ctx_(new command_ctx<Seq, Output>{
		std::forward<Seq>(seq), std::move(pl), buffer_size}) {
    }

    command_seq(const command_seq&) = default;
    command_seq& operator=(const command_seq&) = default;
    command_seq(command_seq&&) = default;
    command_seq& operator=(command_seq&&) = default;

    auto begin() {
	return ctx_->begin();
    }

    auto end() {
	return ctx_->end();
    }

    auto begin() const {
	return const_cast<command_ctx<Seq, Output>*>(ctx_.get())->begin();
    }

    auto end() const {
	return const_cast<command_ctx<Seq, Output>*>(ctx_.get())->end();
    }
};


template <typename Output>
class command_builder {
public:
    command_builder(c7::pipeline&& pl, size_t buffer_size):
	pl_(std::move(pl)), buffer_size_(buffer_size) {}

    command_builder(size_t buffer_size):
	buffer_size_(buffer_size) {}

    command_builder(command_builder&&) = default;
    command_builder& operator=(command_builder&&) = default;

    command_builder& add(const c7::strvec& argv) & {
	pl_.add("", argv[0], argv);
	return *this;
    }

    command_builder& add(const c7::strvec& argv, const c7::strvec& envv) & {
	pl_.add("", argv[0], argv, envv);
	return *this;
    }

    command_builder& add(const std::string& prog, const c7::strvec& argv) & {
	pl_.add("", prog, argv);
	return *this;
    }

    command_builder& add(const std::string& prog, const c7::strvec& argv, const c7::strvec& envv) & {
	pl_.add("", prog, argv, envv);
	return *this;
    }

    command_builder&& add(const c7::strvec& argv) && {
	pl_.add("", argv[0], argv);
	return std::move(*this);
    }

    command_builder&& add(const c7::strvec& argv, const c7::strvec& envv) && {
	pl_.add("", argv[0], argv, envv);
	return std::move(*this);
    }

    command_builder&& add(const std::string& prog, const c7::strvec& argv) && {
	pl_.add("", prog, argv);
	return std::move(*this);
    }

    command_builder&& add(const std::string& prog, const c7::strvec& argv, const c7::strvec& envv) && {
	pl_.add("", prog, argv, envv);
	return std::move(*this);
    }

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return command_seq<decltype(seq), Output>
	    (std::forward<Seq>(seq), std::move(pl_), buffer_size_);
    }

    auto operator()() {
	empty_seq<> seq;
	return command_seq<decltype(seq), Output>
	    (seq, std::move(pl_), buffer_size_);
    }

private:
    c7::pipeline pl_;
    size_t buffer_size_;
};


template <typename Output>
auto command(size_t buffer_size=8192)
{
    return command_builder<Output>(buffer_size);
}

template <typename Output>
auto command(const c7::strvec& argv, size_t buffer_size=8192)
{
    auto c = command_builder<Output>(buffer_size);
    c.add(argv);
    return c;
}

template <typename Output>
auto command(const c7::strvec& argv, const c7::strvec& envv, size_t buffer_size=8192)
{
    auto c = command_builder<Output>(buffer_size);
    c.add(argv, envv);
    return c;
}

template <typename Output>
auto command(const std::string& prog, const c7::strvec& argv, size_t buffer_size=8192)
{
    auto c = command_builder<Output>(buffer_size);
    c.add(prog, argv);
    return c;
}

template <typename Output>
auto command(const std::string& prog, const c7::strvec& argv, const c7::strvec& envv, size_t buffer_size=8192)
{
    auto c = command_builder<Output>(buffer_size);
    c.add(prog, argv, envv);
    return c;
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq, typename Output>
struct format_ident<c7::nseq::command_seq<Seq, Output>> {
    static constexpr const char *name = "command";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/command.hpp
