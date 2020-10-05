/*
 * c7threadcom.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_THREADCOM_HPP_LOADED__
#define C7_THREADCOM_HPP_LOADED__
#include <c7common.hpp>


#include <c7threadsync.hpp>
#include <type_traits>


namespace c7 {
namespace thread {


/*----------------------------------------------------------------------------
                          mono-directional 1 element
----------------------------------------------------------------------------*/

template <typename T>
class pipe1 {
private:
    enum status_t {
	EMPTY, EXIST, CLOSING, CLOSED, ABORTED
    };

    condvar cv_;
    status_t state_ = EMPTY;
    T data_;
    size_t put_n_ = 0;
    size_t get_n_ = 0;

    template <typename Assign>
    com_status put_framework(::timespec *tmp, Assign assign);

public:
    pipe1(const pipe1<T>&) = delete;
    pipe1<T>& operator=(const pipe1<T>&) = delete;

    pipe1() {}

    pipe1(pipe1<T>&& o):
	cv_(std::move(o.cv_)), state_(o.state_), data_(std::move(o.data_)),
	put_n_(o.put_n_), get_n_(o.get_n_) {
    }

    pipe1<T>& operator=(pipe1<T>&& o);

    com_status put(const T& data, ::timespec *tmp = nullptr) {
	return put_framework(tmp, [this, &data](){ data_ = data; });
    }

    com_status put(T&& data, ::timespec *tmp = nullptr) {
	return put_framework(tmp, [this, &data](){ data_ = std::move(data); });
    }

    com_status flush(...) {
	return com_status::OK;
    }

    com_status get(T& data, ::timespec *tmp = nullptr);
    void close();
    void abort();
    void reset();

    size_t put_n() const {
	return put_n_;
    }

    size_t get_n() const {
	return get_n_;
    }
};

template <typename T>
template <typename Assign>
com_status pipe1<T>::put_framework(::timespec *tmp, Assign assign)
{
    auto defer = cv_.lock();
    while (state_ == EXIST) {
	if (!cv_.wait(tmp)) {
	    return com_status::TIMEOUT;
	}
    }
    if (state_ == EMPTY) {
	state_ = EXIST;
	assign();
	put_n_++;
	cv_.notify_all();
	return com_status::OK;
    }
    if (state_ == CLOSING || state_ == CLOSED) {
	return com_status::CLOSED;
    }
    return com_status::ABORT;
}

template <typename T>
com_status pipe1<T>::get(T& data, ::timespec *tmp)
{
    auto defer = cv_.lock();
    while (state_ == EMPTY) {
	if (!cv_.wait(tmp))
	    return com_status::TIMEOUT;
    }
    if (state_ == EXIST || state_ == CLOSING) {
	data = std::move(data_);
	state_ = (state_ == EXIST) ? EMPTY : CLOSED;
	get_n_++;
	cv_.notify_all();
	return com_status::OK;
    }
    if (state_ == CLOSED) {
	return com_status::CLOSED;
    }
    return com_status::ABORT;
}

template <typename T>
pipe1<T>& pipe1<T>::operator=(pipe1<T>&& o)
{
    if (this != &o) {
	cv_    = std::move(o.cv_);
	state_ = o.state_;
	data_  = std::move(o.data_);
	put_n_ = o.put_n_;
	get_n_ = o.get_n_;
    }
    return *this;
}

template <typename T>
void pipe1<T>::close()
{
    auto defer = cv_.lock();
    if (state_ == EMPTY)
	state_ = CLOSED;
    else if (state_ == EXIST)
	state_ = CLOSING;
    cv_.notify_all();
}

template <typename T>
void pipe1<T>::abort()
{
    auto defer = cv_.lock();
    if (state_ != ABORTED) {
	state_ = ABORTED;
	cv_.notify_all();
    }
}

template <typename T>
void pipe1<T>::reset()
{
    auto defer = cv_.lock();
    if (state_ == EXIST || state_ == CLOSING) {
	auto d = std::move(data_);
	(void)d;
    }
    state_ = EMPTY;
    put_n_ = get_n_ = 0;
}


/*----------------------------------------------------------------------------
                   mono-directional, fixed size dual buffer
----------------------------------------------------------------------------*/

template <typename T>
class fpipe {
private:
    struct buf {
	T* top;
	uint32_t cnt;
	uint32_t idx;
    };

    uint32_t n_elm_;
    std::unique_ptr<T[]> storage_;
    pipe1<buf*> put2get_, get2put_;
    buf bufs_[2];
    buf *put_;
    buf *get_;
    size_t put_n_ = 0;
    size_t get_n_ = 0;
    
    com_status put_ready(::timespec * tmp);

public:
    fpipe(const fpipe&) = delete;
    fpipe<T>& operator=(const fpipe&) = delete;

    fpipe(fpipe<T>&& o):
	n_elm_(o.n_elm_), storage_(std::move(o.storage_)),
	put2get_(std::move(o.put2get_)), get2put_(std::move(o.get2put_)) {
	reset();
    }
    fpipe<T>& operator=(fpipe<T>&& o);

    fpipe(): n_elm_(0), storage_() {
	reset();
    }

    explicit fpipe(int n_elm): n_elm_(n_elm), storage_(new T[n_elm * 2]) {
	reset();
    }

    void init(int n_elm);
    void reset();
    com_status put(const T& data, ::timespec *tmp = nullptr);
    com_status put(T&& data, ::timespec *tmp = nullptr);
    com_status flush(::timespec *tmp = nullptr);
    com_status get(T& data, ::timespec *tmp = nullptr);
    com_status close(::timespec *tmp = nullptr);

    void abort() {
	get2put_.abort();
	put2get_.abort();
    }

    size_t put_n() const {
	return put_n_;
    }

    size_t get_n() const {
	return get_n_;
    }
};

template <typename T>
fpipe<T>& fpipe<T>::operator=(fpipe<T>&& o)
{
    if (this != &o) {
	n_elm_   = o.n_elm_;
	storage_ = std::move(o.storage_);
	put2get_ = std::move(o.put2get_);
	get2put_ = std::move(o.get2put_);
	reset();
    }
    return *this;
}

template <typename T>
com_status fpipe<T>::put_ready(::timespec * tmp)
{
    if (put_ != nullptr && put_->idx == put_->cnt) {
	if (auto s = put2get_.put(put_, tmp); s != com_status::OK)
	    return s;
	put_ = nullptr;
    }
    if (put_ == nullptr) {
	if (auto s = get2put_.get(put_, tmp); s != com_status::OK)
	    return s;
	put_->cnt = n_elm_;
	put_->idx = 0;
    }
    return com_status::OK;
}

template <typename T>
void fpipe<T>::init(int n_elm)
{
    n_elm_ = n_elm;
    storage_.reset(new T[n_elm * 2]);
    reset();
}

template <typename T>
void fpipe<T>::reset()
{
    for (auto p = storage_.get(), q = p + n_elm_ * 2; p < q; p++) {
	*p = T();
    }

    bufs_[0].top = storage_.get();
    bufs_[0].cnt = n_elm_;
    bufs_[0].idx = 0;
    put_ = &bufs_[0];
    get2put_.reset();

    bufs_[1].top = bufs_[0].top + n_elm_;
    bufs_[1].cnt = n_elm_;
    bufs_[1].idx = n_elm_;
    get_ = &bufs_[1];
    put2get_.reset();
}

template <typename T>
com_status fpipe<T>::put(const T& data, ::timespec *tmp)
{
    auto s = put_ready(tmp);
    if (s == com_status::OK) {
	put_->top[put_->idx] = data;
	put_->idx++;
	put_n_++;
    }
    return s;
}

template <typename T>
com_status fpipe<T>::put(T&& data, ::timespec *tmp)
{
    auto s = put_ready(tmp);
    if (s == com_status::OK) {
	put_->top[put_->idx] = std::move(data);
	put_->idx++;
	put_n_++;
    }
    return s;
}

template <typename T>
com_status fpipe<T>::flush(::timespec *tmp)
{
    if (put_ == nullptr || put_->idx == 0) {
	return com_status::OK;
    }
    put_->cnt = put_->idx;
    return put_ready(tmp);
}

template <typename T>
com_status fpipe<T>::get(T& data, ::timespec *tmp)
{
    if (get_ != nullptr && get_->idx == get_->cnt) {
	if (auto s = get2put_.put(get_, tmp); s != com_status::OK) {
	    return s;
	}
	get_ = nullptr;
    }
    if (get_ == nullptr) {
	if (auto s = put2get_.get(get_, tmp); s != com_status::OK) {
	    return s;
	}
	get_->idx = 0;
    }
    data = std::move(get_->top[get_->idx]);
    get_->idx++;
    get_n_++;
    return com_status::OK;
}

template <typename T>
com_status fpipe<T>::close(::timespec *tmp)
{
    flush(tmp);
    put2get_.close();
    get2put_.reset();
    return com_status::OK;
}


/*----------------------------------------------------------------------------
                           request-response channel
----------------------------------------------------------------------------*/

template <typename Req, typename Reply,
	  template <typename> class Pipe = pipe1>
class channel {
private:
    Pipe<Req> request_;
    Pipe<Reply> reply_;
    counter user_;

public:
    channel(const channel<Req, Reply>&) = delete;
    channel<Req, Reply>& operator=(const channel<Req, Reply>&) = delete;

    channel(channel<Req, Reply>&& o):
	request_(std::move(o.request_)), reply_(std::move(o.reply_)) {
    }
    channel<Req, Reply>& operator=(channel<Req, Reply>&& o) {
	if (this != &o) {
	    request_ = std::move(o.request_);
	    reply_   = std::move(o.reply_);
	}
	return *this;
    }

    channel(): request_(), reply_(), user_() {}

    channel(int n): request_(n), reply_(n), user_() {}

    // for both request sender and request reciever

    void attach() {
	user_.up();
    }

    void detach(bool wait_all_detached = true) {
	(void)user_.down();
	if (wait_all_detached) {
	    (void)user_.wait_just(0);
	}
    }

    void abort() {
	request_.abort();
	reply_.abort();
    }

    // for request sender

    com_status send_request(const Req& data, ::timespec *tmp = nullptr) {
	return request_.put(data, tmp);
    }

    com_status send_request(Req&& data, ::timespec *tmp = nullptr) {
	return request_.put(std::move(data), tmp);
    }

    com_status recv_reply(Reply& data, ::timespec *tmp = nullptr) {
	return reply_.get(data, tmp);
    }

    com_status recv_reply(::timespec *tmp = nullptr) {
	Reply data;
	return reply_.get(data, tmp);
    }

    bool has_unconfirmed() {
	return (request_.put_n() != reply_.get_n());
    }

    void close_request() {
	request_.close();
    }

    void reset() {
	request_.reset();
	reply_.reset();
    }

    // for request receiver

    com_status recv_request(Req& data, ::timespec *tmp = nullptr) {
	return request_.get(data, tmp);
    }

    com_status send_reply(const Reply& data, ::timespec *tmp = nullptr) {
	return reply_.put(data, tmp);
    }

    com_status send_reply(Reply&& data, ::timespec *tmp = nullptr) {
	return reply_.put(std::move(data), tmp);
    }

    void close_reply() {
	reply_.close();
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace thread
} // namespace c7


#endif // c7threadcom.hpp
