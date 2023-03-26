#if defined(C7_NSEQ_ITEREND_TYPE__)
    bool operator==(const C7_NSEQ_ITEREND_TYPE__&) const {
	return (it_ == itend_);
    }

    bool operator!=(const C7_NSEQ_ITEREND_TYPE__& o) const {
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

    bool operator<(const C7_NSEQ_ITER_TYPE__& o) const {
	return it_ < o.it_;
    }

    bool operator<=(const C7_NSEQ_ITER_TYPE__& o) const {
	return it_ <= o.it_;
    }

    bool operator>(const C7_NSEQ_ITER_TYPE__& o) const {
	return o < *this;
    }

    bool operator>=(const C7_NSEQ_ITER_TYPE__& o) const {
	return o <= *this;
    }

#if defined(C7_NSEQ_ITEREND_TYPE__)
    bool operator<(const C7_NSEQ_ITEREND_TYPE__&) const {
	return it_ < itend_;
    }

    bool operator<=(const C7_NSEQ_ITEREND_TYPE__&) const {
	return it_ <= itend_;
    }

    bool operator>(const C7_NSEQ_ITEREND_TYPE__&) const {
	return it_ > itend_;
    }

    bool operator>=(const C7_NSEQ_ITEREND_TYPE__&) const {
	return it_ >= itend_;
    }

    friend
    bool operator==(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b == a;
    }

    friend
    bool operator!=(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b != a;
    }

    friend
    bool operator<(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b > a;
    }

    friend
    bool operator<=(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b >= a;
    }

    friend
    bool operator>(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b < a;
    }

    friend
    bool operator>=(const C7_NSEQ_ITEREND_TYPE__& a, const C7_NSEQ_ITER_TYPE__& b) {
	return b <= a;
    }
#endif

#undef C7_NSEQ_ITER_TYPE__
#undef C7_NSEQ_ITEREND_TYPE__
