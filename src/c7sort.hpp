/*
 * c7sort.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=612923095
 */

#ifndef C7_SORT_HPP_LOADED__
#define C7_SORT_HPP_LOADED__
#include <c7common.hpp>


#include <any>		// std::swap
#include <functional>	// std::function
#include <iterator>	// std::iterator_traits
#include <thread>


namespace c7 {


#define C7_MSORT_MAX_DEPTH	(64)
#define C7_QSORT_MAX_DEPTH	(128)


/*----------------------------------------------------------------------------
                            direct selection sort
----------------------------------------------------------------------------*/

template <typename Iterator, typename Lessthan>
static void dsort(Iterator left, Iterator right, Lessthan lessthan)
{
    auto p = left, q = right;
    do {
	for (p++; p <= right; p++) {
	    if (lessthan(p[0], p[-1])) {
		auto tmp = *p;
		q = p;
		do {
		    *q = *(q - 1);
		    q--;
		} while (q > left && lessthan(tmp, q[-1]));
		*q = tmp;
	    }
	}
    } while (0);
}

template <typename Iterator>
static void dsort(Iterator left, Iterator right)	// right point last item
{
    dsort(left, right,
	  [](const decltype(*left)& p,
	     const decltype(*left)& q) { return p < q; });
}


/*----------------------------------------------------------------------------
                                  heap sort
----------------------------------------------------------------------------*/

static inline ptrdiff_t heap_left_child(ptrdiff_t i)
{
    return (i << 1) + 1;
}

static inline ptrdiff_t heap_right_child(ptrdiff_t i)
{
    return (i << 1) + 2;
}

static inline ptrdiff_t heap_parent(ptrdiff_t i)
{
    return (i - 1) >> 1;
}

template <typename Iterator, typename Lessthan>
static void heap_up(Iterator& array, ptrdiff_t c, Lessthan lessthan)
{
    ptrdiff_t c_save = c;
    auto target = array[c];

    while (c > 0) {
	ptrdiff_t p = heap_parent(c);
	if (lessthan(array[p], target)) {
	    array[c] = array[p];
	} else {
	    break;
	}
	c = p;
    }
    if (c != c_save) {
	array[c] = target;
    }
}

template <typename Iterator, typename Lessthan>
static void heap_down(Iterator& array, ptrdiff_t n, Lessthan lessthan,
		      decltype(*array)& target)
{
    ptrdiff_t p = 0;

    for (;;) {
	ptrdiff_t c = heap_left_child(p);
	if (c >= n) {
	    break;
	}
	if (((c + 1) < n) && lessthan(array[c], array[c+1])) {
	    c = c + 1;
	}
	if (!lessthan(target, array[c])) {
	    break;
	}
	array[p] = array[c];
	p = c;
    }
    array[p] = target;
}

template <typename Iterator, typename Lessthan>
void hsort_st(Iterator left, ptrdiff_t n, Lessthan lessthan)
{
    ptrdiff_t i = 0;
    
    while (++i < n) {
	heap_up(left, i, lessthan);
    }

    while (--i > 0) {
	auto tmp = left[i];
	left[i] = left[0];
	heap_down(left, i, lessthan, tmp);
    }
}

template <typename Iterator>
void hsort_st(Iterator left, ptrdiff_t n)
{
    hsort_st(left, n,
	     [](const decltype(*left)& p,
		const decltype(*left)& q) { return p < q; });
}


/*----------------------------------------------------------------------------
                          merge sort - single thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Lessthan>
static void msort_merge(Iterator o, Iterator p, Iterator q, Iterator eq, Lessthan lessthan)
{
    Iterator ep = q;
    for (;;) {
	if (lessthan(*p, *q)) {
	    *o++ = *p++;
	    if (p == ep) {
		p = q;
		ep = eq;
		break;
	    }
	} else {
	    *o++ = *q++;
	    if (q == eq) {
		break;
	    }
	}
    }
    while (p < ep) {
	*o++ = *p++;
    }
}

template <typename Iterator, typename Lessthan>
static void msort_st_main(int parity, Iterator out, Iterator in, ptrdiff_t n,
			  Lessthan lessthan, const ptrdiff_t msort_threshold)
{
    enum op_t { OP_SORT, OP_MERGE };
    struct {
	op_t op;
	int parity;
	Iterator out;
	Iterator in;
	ptrdiff_t n;
    } stack[C7_MSORT_MAX_DEPTH * 2];
    int stack_idx = 0;
    auto op = OP_SORT;

    for (;;) {
	ptrdiff_t h = n >> 1;

	if (op == OP_SORT) {
	    if ((n < msort_threshold) && (parity == 0)) {
		dsort(out, out + (n - 1), lessthan);
		if (stack_idx == 0)
		    break;
		stack_idx--;
		op = stack[stack_idx].op;
		parity = stack[stack_idx].parity;
		out = stack[stack_idx].out;
		in = stack[stack_idx].in;
		n = stack[stack_idx].n;
	    } else {
		if ((stack_idx + 2) > c7_numberof(stack)) {
		    abort();
		}
		stack[stack_idx].op = OP_MERGE;
		stack[stack_idx].parity = parity;
		stack[stack_idx].out = out;
		stack[stack_idx].in = in;
		stack[stack_idx].n = n;
		stack_idx++;
		stack[stack_idx].op = OP_SORT;
		stack[stack_idx].parity = !parity;
		stack[stack_idx].out = in + h;
		stack[stack_idx].in = out + h;
		stack[stack_idx].n = n - h;
		stack_idx++;

		/* continue sort left half */
		parity = !parity;
		std::swap(in, out);
		n = h;
	    }
	} else {	// OP_MERGE
	    msort_merge(out, in, in+h, in+n, lessthan);
	    if (stack_idx == 0) {
		break;
	    }
	    stack_idx--;
	    op = stack[stack_idx].op;
	    parity = stack[stack_idx].parity;
	    out = stack[stack_idx].out;
	    in = stack[stack_idx].in;
	    n = stack[stack_idx].n;
	}
    }
}

template <typename Iterator, typename Lessthan>
void msort_st(Iterator work,
	      Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int msort_threshold = 50)
{
    msort_st_main(0, left, work, n, lessthan, msort_threshold);
}

template <typename Iterator>
void msort_st(Iterator work,
	      Iterator left, ptrdiff_t n,
	      int msort_threshold = 50)
{
    msort_st_main(0, left, work, n,
		  [](const decltype(*left)& p,
		     const decltype(*left)& q) { return p < q; },
		  msort_threshold);
}


/*----------------------------------------------------------------------------
                          merge sort - multi thread
----------------------------------------------------------------------------*/

template <typename Iterator>
struct msort_param_t {
    Iterator out;
    Iterator in;
    ptrdiff_t n;
    ptrdiff_t h;
    ptrdiff_t threshold;
    int parity;
    int level;
};

template <typename Iterator, typename Lessthan>
static void msort_merge_asc(const msort_param_t<Iterator>* const ms, Lessthan lessthan)
{
    Iterator o = ms->out;
    Iterator eo = o + ms->h;
    Iterator p = ms->in;
    Iterator q = p + ms->h;

    while (o != eo) {
	if (!lessthan(*q, *p)) {		/* !(*q < *p) <=> *p <= *q */
	    *o++ = *p++;
	} else {
	    *o++ = *q++;
	}
    }
}

template <typename Iterator, typename Lessthan>
static void msort_merge_dsc(const msort_param_t<Iterator>* const ms, Lessthan lessthan)
{
    Iterator o = (Iterator )ms->out + (ms->n - 1);
    Iterator bo = (Iterator )ms->out + ms->h;
    Iterator p = (Iterator )ms->in + (ms->h - 1);
    Iterator q = (Iterator )ms->in + (ms->n - 1);

    while (o != bo) {
	if (!lessthan(*q, *p)) {		/* !(*q < *p) <=> *p <= *q */
	    *o-- = *q--;
	} else {
	    *o-- = *p--;
	}
    }
    if ((p == (ms->in - 1)) || lessthan(*p, *q)) {
	*o = *q;
    } else {
	*o = *p;
    }
}

template <typename Iterator, typename Lessthan>
static void msort_mt_main(const msort_param_t<Iterator>* const ms, Lessthan lessthan)
{
    msort_param_t<Iterator> ms1, ms2;

    if ((ms->level == 0) || (ms->n < ms->threshold)) {
	msort_st_main(ms->parity, ms->out, ms->in, ms->n, lessthan, ms->threshold);
	return;
    }

    ms1.out = ms->in;
    ms1.in = ms->out;
    ms1.n = ms->h;
    ms1.h = ms1.n >> 1;
    ms1.threshold = ms->threshold;
    ms1.parity = !ms->parity;
    ms1.level = ms->level - 1;

    ms2.out = ms->in + ms->h;
    ms2.in = ms->out + ms->h;
    ms2.n = ms->n - ms->h;
    ms2.h = ms2.n >> 1;
    ms2.threshold = ms->threshold;
    ms2.parity = !ms->parity;
    ms2.level = ms->level - 1;

    std::thread th1(msort_mt_main<Iterator, Lessthan>, &ms1, lessthan);
    msort_mt_main(&ms2, lessthan);
    th1.join();

    ms1 = *ms;		// ms1 is reused for msort_merge_xxx
    std::thread th2(msort_merge_asc<Iterator, Lessthan>, &ms1, lessthan);
    msort_merge_dsc(&ms1, lessthan);
    th2.join();
}

template <typename Iterator, typename Lessthan>
void msort_mt(Iterator work,
	      Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int thread_depth, int msort_threshold = 50)
{
    msort_param_t<Iterator> ms;
    ms.out = left;
    ms.in = work;
    ms.n = n;
    ms.h = ms.n >> 1;
    ms.threshold = msort_threshold;
    ms.parity = 0;
    ms.level = thread_depth;
    msort_mt_main(&ms, lessthan);
}

template <typename Iterator>
void msort_mt(Iterator work,
	      Iterator left, ptrdiff_t n, 
	      int thread_depth, int msort_threshold = 50)
{
    msort_mt(work,
	     left, n, 
	     [](const decltype(*left)& p,
		const decltype(*left)& q) { return p < q; },
	     thread_depth, msort_threshold);
}


/*----------------------------------------------------------------------------
                          quick sort - single thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Lessthan>
static void qsort_st_main(Iterator left, Iterator right, Lessthan lessthan,
			  int qsort_threshold)
{
    struct {
	Iterator left;
	Iterator right;
    } stack[C7_QSORT_MAX_DEPTH];
    int stack_idx = 0;

    for (;;) {
	ptrdiff_t n = right - left;
	if (n < qsort_threshold) {
	    dsort(left, right, lessthan);
	    if (stack_idx == 0)
		break;
	    stack_idx--;
	    left = stack[stack_idx].left;
	    right = stack[stack_idx].right;
	    continue;
	}

	// right - left >= C7_QSORT_THRESHOLD

	typename std::iterator_traits<Iterator>::value_type s;
	{
	    auto v0 = left [     1];
	    auto v1 = left [n >> 1];
	    auto v2 = right[    -1];
	    if (lessthan(v0, v1)) {
		if (lessthan(v1, v2))
		    s = v1;
		else if (lessthan(v0, v2))
		    s = v2;
		else
		    s = v0;
	    } else {
		if (lessthan(v0, v2))
		    s = v0;
		else if (lessthan(v1, v2))
		    s = v2;
		else
		    s = v1;
	    }
	}

	Iterator p = left;
	Iterator q = right;
	do {
	    while (lessthan(*p, s))
		p++;
	    while (lessthan(s, *q))
		q--;
	    if (p <= q) {
		std::swap(*p, *q);
		p++;
		q--;
	    }
	} while (p <= q);

	if (stack_idx == c7_numberof(stack)) {
	    hsort_st(left, p - left, lessthan);
	    hsort_st(p, (right + 1) - p, lessthan);
	    stack_idx--;
	    left = stack[stack_idx].left;
	    right = stack[stack_idx].right;
	} else {
	    stack[stack_idx].left = p;
	    stack[stack_idx].right = right;
	    stack_idx++;
	    right = p - 1;
	}
    }
}

template <typename Iterator, typename Lessthan>
void qsort_st(Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int qsort_threshold = 60)
{
    qsort_st_main(left, (left + n) -1, lessthan, qsort_threshold);
}

template <typename Iterator>
void qsort_st(Iterator left, ptrdiff_t n,
	      int qsort_threshold = 60)
{
    qsort_st_main(left, (left + n) -1,
		  [](const decltype(*left)& p,
		     const decltype(*left)& q) { return p < q; },
		  qsort_threshold);
}


/*----------------------------------------------------------------------------
                          quick sort - multi thread
----------------------------------------------------------------------------*/

template <typename Iterator>
struct qsort_param_t {
    Iterator left;
    Iterator right;
    ptrdiff_t threshold;
    int level;
};

template <typename Iterator, typename Lessthan>
static void qsort_mt_main(const qsort_param_t<Iterator>* const qs, Lessthan lessthan)
{
    ptrdiff_t n = qs->right - qs->left;
    if (qs->level == 0 || n < qs->threshold) {
	qsort_st_main(qs->left, qs->right, lessthan, qs->threshold);
	return;
    }

    // right - left >= C7_QSORT_THRESHOLD

    typename std::iterator_traits<Iterator>::value_type s;
    {
	auto v0 = qs->left [     1];
	auto v1 = qs->left [n >> 1];
	auto v2 = qs->right[    -1];
	if (lessthan(v0, v1)) {
	    if (lessthan(v1, v2))
		s = v1;
	    else if (lessthan(v0, v2))
		s = v2;
	    else
		s = v0;
	} else {
	    if (lessthan(v0, v2))
		s = v0;
	    else if (lessthan(v1, v2))
		s = v2;
	    else
		s = v1;
	}
    }

    Iterator p = qs->left;
    Iterator q = qs->right;
    do {
	while (lessthan(*p, s))
	    p++;
	while (lessthan(s, *q))
	    q--;
	if (p <= q) {
	    std::swap(*p, *q);
	    p++;
	    q--;
	}
    } while (p <= q);

    std::thread th;
    qsort_param_t<Iterator> qs1, qs2;

    if (qs->left < (p - 1)) {
	qs1.left = qs->left;
	qs1.right = p - 1;
	qs1.threshold = qs->threshold;
	qs1.level = qs->level - 1;
	th = std::thread(qsort_mt_main<Iterator, Lessthan>, &qs1, lessthan);
    }
    if (p < qs->right) {
	qs2.left = p;
	qs2.right = qs->right;
	qs2.threshold = qs->threshold;
	qs2.level = qs->level - 1;
	qsort_mt_main(&qs2, lessthan);
    }
    if (qs->left < (p - 1)) {
	th.join();
    }
}

template <typename Iterator, typename Lessthan>
void qsort_mt(Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int thread_depth, int qsort_threshold = 60)
{
    qsort_param_t<Iterator> qs;
    qs.left = left;
    qs.right = left + (n - 1);
    qs.threshold = qsort_threshold;
    qs.level = thread_depth;
    qsort_mt_main(&qs, lessthan);
}

template <typename Iterator>
void qsort_mt(Iterator left, ptrdiff_t n,
	      int thread_depth, int qsort_threshold = 60)
{
    qsort_mt(left, n,
	     [](const decltype(*left)& p,
		const decltype(*left)& q) { return p < q; },
	     thread_depth, qsort_threshold);
}


/*----------------------------------------------------------------------------
                          radix sort - single thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Masktype, typename Getkey, typename Lessthan>
static void rsort_st_main(Iterator left, Iterator right, Masktype keymask, Masktype tstmask,
			  Getkey getkey, Lessthan lessthan,
			  int rsort_threshold)
{
    struct {
	Iterator left;
	Iterator right;
	Masktype tstmask;
    } stack[sizeof(uint64_t) * 8];
    int stack_idx = 0;

    // find start bit
    for (; tstmask != 0 && (keymask & tstmask) == 0; tstmask >>= 1) {;}
    if (tstmask == 0)
	return;

    for (;;) {
	if (right - left < rsort_threshold) {
	    dsort(left, right, lessthan);
	    if (stack_idx == 0)
		return;
	    // pop
	    stack_idx--;
	    left = stack[stack_idx].left;
	    right = stack[stack_idx].right;
	    tstmask = stack[stack_idx].tstmask;
	    continue;
	}

	Iterator p = left;
	Iterator q = right;
	while (p <= q && (getkey(*p) & tstmask) == 0)
	    p++;
	while (p <= q && (getkey(*q) & tstmask) != 0)
	    q--;
	while (p < q) {
	    std::swap(*p, *q);
	    p++;
	    q--;
	    while ((getkey(*p) & tstmask) == 0)
		p++;
	    while ((getkey(*q) & tstmask) != 0)
		q--;
	}

	// find next lower bit
	for (tstmask >>= 1; tstmask != 0 && (keymask & tstmask) == 0; tstmask >>= 1) {;}
	if (tstmask == 0) {
	    if (stack_idx == 0)
		return;
	    stack_idx--;
	    left = stack[stack_idx].left;
	    right = stack[stack_idx].right;
	    tstmask = stack[stack_idx].tstmask;
	} else {
	    /* push: sort p..right */
	    stack[stack_idx].left = p;
	    stack[stack_idx].right = right;
	    stack[stack_idx].tstmask = tstmask;
	    stack_idx++;
	    /* sort left..q */
	    right = q;
	}
    }
}

template <typename Iterator, typename Masktype, typename Getkey>
void rsort_st(Iterator left, ptrdiff_t n, Masktype keymask, Getkey getkey,
	      int rsort_threshold = 50)
{
    Masktype tstmask = ((Masktype)1) << (sizeof(Masktype)*8 - 1);
    rsort_st_main(left, left + n -1, keymask, tstmask, getkey,
		  [&getkey, keymask](const decltype(*left)& p,
				     const decltype(*left)& q) {
		      return ((getkey(p) & keymask) < (getkey(q) & keymask));
		  },
		  rsort_threshold);
}


/*----------------------------------------------------------------------------
                          radix sort - multi thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Masktype>
struct rsort_prm_t {
    Iterator left;
    Iterator right;
    ptrdiff_t threshold;
    Masktype keymask;
    Masktype tstmask;
    int level;
};

template <typename Iterator, typename Masktype, typename Getkey, typename Lessthan>
static void rsort_mt_main(const rsort_prm_t<Iterator, Masktype> *rs,
			  Getkey getkey, Lessthan lessthan)
{
    Iterator p = rs->left;
    Iterator q = rs->right;
    Masktype keymask = rs->keymask;
    Masktype tstmask = rs->tstmask;

    if (rs->level == 0 || (q - p) < rs->threshold) {
	rsort_st_main(p, q, keymask, tstmask, getkey, lessthan, rs->threshold);
	return;
    }

    while (p <= q && (getkey(*p) & tstmask) == 0)
	p++;
    while (p <= q && (getkey(*q) & tstmask) != 0)
	q--;
    while (p < q) {
	std::swap(*p, *q);
	p++;
	q--;
	while ((getkey(*p) & tstmask) == 0)
	    p++;
	while ((getkey(*q) & tstmask) != 0)
	    q--;
    }

    /* find next lower bit */
    for (tstmask >>= 1; tstmask != 0 && (keymask & tstmask) == 0; tstmask >>= 1) {;}
    if (tstmask == 0)
	return;

    std::thread th;
    rsort_prm_t<Iterator, Masktype> rs1, rs2;

    if (rs->left < q) {
	rs1.left = rs->left;
	rs1.right = q;
	rs1.threshold = rs->threshold;
	rs1.level = rs->level - 1;
	rs1.keymask = keymask;
	rs1.tstmask = tstmask;
	th = std::thread(rsort_mt_main<Iterator, Masktype, Getkey, Lessthan>, &rs1, getkey, lessthan);
    }
    if (p < rs->right) {
	rs2.left = p;
	rs2.right = rs->right;
	rs2.threshold = rs->threshold;
	rs2.level = rs->level - 1;
	rs2.keymask = keymask;
	rs2.tstmask = tstmask;
	rsort_mt_main(&rs2, getkey, lessthan);
    }
    if (rs->left < q)
	th.join();
}

template <typename Iterator, typename Masktype, typename Getkey>
void rsort_mt(Iterator left, ptrdiff_t n, Masktype keymask, Getkey getkey,
	      int thread_depth, int rsort_threshold = 50)
{
    /* find start bit */
    Masktype tstmask = ((Masktype)1) << (sizeof(Masktype)*8 - 1);
    for (; tstmask != 0 && (keymask & tstmask) == 0; tstmask >>= 1) {;}
    if (tstmask == 0)
	return;

    rsort_prm_t<Iterator, Masktype> rs;
    rs.left = left;
    rs.right = left + (n - 1);
    rs.threshold = rsort_threshold;
    rs.level = thread_depth;
    rs.keymask = keymask;
    rs.tstmask = tstmask;
    rsort_mt_main(&rs, getkey,
		  [&getkey, keymask](const decltype(*left)& p,
				     const decltype(*left)& q) {
		      return ((getkey(p) & keymask) < (getkey(q) & keymask));
		  });
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7sort.hpp
