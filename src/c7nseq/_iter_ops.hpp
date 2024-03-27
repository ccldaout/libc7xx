#if defined(C7_NSEQ_ITEREND_TYPE_)
    bool operator==(const C7_NSEQ_ITEREND_TYPE_&) const {
	return (it_ == itend_);
    }

    bool operator!=(const C7_NSEQ_ITEREND_TYPE_& o) const {
	return !(*this == o);
    }
#endif

    auto operator++(int) {	// postfix
	auto cur = *this;
	++(*this);
	return cur;
    }

    auto operator--(int) {	// postfix
	auto cur = *this;
	--(*this);
	return cur;
    }

    auto& operator-=(ptrdiff_t n) {
	(*this)+=(-n);
	return *this;
    }

    auto operator-(ptrdiff_t n) const {
	return (*this) + (-n);
    }

    bool operator<(const C7_NSEQ_ITER_TYPE_& o) const {
	return it_ < o.it_;
    }

    bool operator<=(const C7_NSEQ_ITER_TYPE_& o) const {
	return it_ <= o.it_;
    }

    bool operator>(const C7_NSEQ_ITER_TYPE_& o) const {
	return o < *this;
    }

    bool operator>=(const C7_NSEQ_ITER_TYPE_& o) const {
	return o <= *this;
    }

#if defined(C7_NSEQ_ITEREND_TYPE_)
    bool operator<(const C7_NSEQ_ITEREND_TYPE_&) const {
	return it_ < itend_;
    }

    bool operator<=(const C7_NSEQ_ITEREND_TYPE_&) const {
	return it_ <= itend_;
    }

    bool operator>(const C7_NSEQ_ITEREND_TYPE_&) const {
	return it_ > itend_;
    }

    bool operator>=(const C7_NSEQ_ITEREND_TYPE_&) const {
	return it_ >= itend_;
    }

    friend
    bool operator==(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b == a;
    }

    friend
    bool operator!=(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b != a;
    }

    friend
    bool operator<(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b > a;
    }

    friend
    bool operator<=(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b >= a;
    }

    friend
    bool operator>(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b < a;
    }

    friend
    bool operator>=(const C7_NSEQ_ITEREND_TYPE_& a, const C7_NSEQ_ITER_TYPE_& b) {
	return b <= a;
    }
#endif

#undef C7_NSEQ_ITER_TYPE_
#undef C7_NSEQ_ITEREND_TYPE_
