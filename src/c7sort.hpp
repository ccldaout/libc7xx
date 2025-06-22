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

#ifndef C7_SORT_HPP_LOADED_
#define C7_SORT_HPP_LOADED_
#include <c7common.hpp>


#include <any>		// std::swap
#include <functional>	// std::function
#include <iterator>	// std::iterator_traits
#include <thread>


#define C7_MSORT_MAX_DEPTH	(64)
#define C7_QSORT_MAX_DEPTH	(128)

#if !defined(C7_MSORT_THRESHOLD)
# define C7_MSORT_THRESHOLD	50
#endif
#if !defined(C7_MSORT_MT_THRESHOLD)
# define C7_MSORT_MT_THRESHOLD	10000
#endif

#if !defined(C7_QSORT_THRESHOLD)
# define C7_QSORT_THRESHOLD	60
#endif
#if !defined(C7_QSORT_MT_THRESHOLD)
# define C7_QSORT_MT_THRESHOLD	10000
#endif

#if !defined(C7_RSORT_THRESHOLD)
# define C7_RSORT_THRESHOLD	50
#endif
#if !defined(C7_RSORT_MT_THRESHOLD)
# define C7_RSORT_MT_THRESHOLD	10000
#endif


namespace c7 {


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
		auto tmp = std::move(*p);
		q = std::move(p);
		do {
		    *q = std::move(*(q - 1));
		    q--;
		} while (q > left && lessthan(tmp, q[-1]));
		*q = std::move(tmp);
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
    auto target = std::move(array[c]);

    while (c > 0) {
	ptrdiff_t p = heap_parent(c);
	if (lessthan(array[p], target)) {
	    array[c] = std::move(array[p]);
	    c = p;
	} else {
	    break;
	}
    }
    array[c] = std::move(target);
}

template <typename Iterator, typename Lessthan>
static void heap_down(Iterator& array, ptrdiff_t n, ptrdiff_t p, Lessthan lessthan)
{
    auto target = std::move(array[p]);

    for (;;) {
	ptrdiff_t c = heap_left_child(p);
	if (c >= n) {
	    break;
	}
	if (((c + 1) < n) && lessthan(array[c], array[c+1])) {
	    c = c + 1;
	}
	if (lessthan(array[c], target)) {
	    break;
	}
	array[p] = std::move(array[c]);
	p = c;
    }
    array[p] = std::move(target);
}

template <typename Iterator, typename Lessthan>
void hsort_st(Iterator left, ptrdiff_t n, Lessthan lessthan)
{
    ptrdiff_t i = 0;

    while (++i < n) {
	heap_up(left, i, lessthan);
    }

    while (--i > 0) {
	using std::swap;
	swap(left[0], left[i]);
	heap_down(left, i, 0, lessthan);
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
	    *o++ = std::move(*p++);
	    if (p == ep) {
		p = q;
		ep = eq;
		break;
	    }
	} else {
	    *o++ = std::move(*q++);
	    if (q == eq) {
		break;
	    }
	}
    }
    while (p < ep) {
	*o++ = std::move(*p++);
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
		using std::swap;
		swap(in, out);
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
	      int msort_threshold = C7_MSORT_THRESHOLD)
{
    msort_st_main(0, left, work, n, lessthan, msort_threshold);
}

template <typename Iterator>
void msort_st(Iterator work,
	      Iterator left, ptrdiff_t n,
	      int msort_threshold = C7_MSORT_THRESHOLD)
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
    ptrdiff_t mt_threshold;
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

template <bool ParallelMerge, typename Iterator, typename Lessthan>
static void msort_mt_main(const msort_param_t<Iterator>* const ms, Lessthan lessthan)
{
    msort_param_t<Iterator> ms1, ms2;

    if ((ms->level == 0) || (ms->n < ms->mt_threshold)) {
	msort_st_main(ms->parity, ms->out, ms->in, ms->n, lessthan, ms->threshold);
	return;
    }

    ms1.out = ms->in;
    ms1.in = ms->out;
    ms1.n = ms->h;
    ms1.h = ms1.n >> 1;
    ms1.threshold = ms->threshold;
    ms1.mt_threshold = ms->mt_threshold;
    ms1.parity = !ms->parity;
    ms1.level = ms->level - 1;

    ms2.out = ms->in + ms->h;
    ms2.in = ms->out + ms->h;
    ms2.n = ms->n - ms->h;
    ms2.h = ms2.n >> 1;
    ms2.threshold = ms->threshold;
    ms2.mt_threshold = ms->mt_threshold;
    ms2.parity = !ms->parity;
    ms2.level = ms->level - 1;

    std::thread th1(msort_mt_main<ParallelMerge, Iterator, Lessthan>, &ms1, lessthan);
    msort_mt_main<ParallelMerge>(&ms2, lessthan);
    th1.join();

    if constexpr (ParallelMerge) {
	ms1 = *ms;		// ms1 is reused for msort_merge_xxx
	std::thread th2(msort_merge_asc<Iterator, Lessthan>, &ms1, lessthan);
	msort_merge_dsc(&ms1, lessthan);
	th2.join();
    } else {
	msort_merge(ms->out, ms->in, ms->in+ms->h, ms->in+ms->n, lessthan);
    }
}

template <typename Iterator, typename Lessthan>
void msort_mt(Iterator work, Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int thread_depth,
	      int msort_mt_threshold = C7_MSORT_MT_THRESHOLD,
	      int msort_threshold = C7_MSORT_THRESHOLD)
{
    msort_param_t<Iterator> ms;
    ms.out = left;
    ms.in = work;
    ms.n = n;
    ms.h = ms.n >> 1;
    ms.threshold = msort_threshold;
    ms.mt_threshold = msort_mt_threshold;
    ms.parity = 0;
    ms.level = thread_depth;
    msort_mt_main<true>(&ms, lessthan);
}

template <typename Iterator>
void msort_mt(Iterator work, Iterator left, ptrdiff_t n,
	      int thread_depth,
	      int msort_mt_threshold = C7_MSORT_MT_THRESHOLD,
	      int msort_threshold = C7_MSORT_THRESHOLD)
{
    msort_mt(work,
	     left, n,
	     [](const decltype(*left)& p,
		const decltype(*left)& q) { return p < q; },
	     thread_depth, msort_mt_threshold, msort_threshold);
}

template <typename Iterator, typename Lessthan>
void msort_mt_mv(Iterator work, Iterator left, ptrdiff_t n, Lessthan lessthan,
		 int thread_depth,
		 int msort_mt_threshold = C7_MSORT_MT_THRESHOLD,
		 int msort_threshold = C7_MSORT_THRESHOLD)
{
    msort_param_t<Iterator> ms;
    ms.out = left;
    ms.in = work;
    ms.n = n;
    ms.h = ms.n >> 1;
    ms.threshold = msort_threshold;
    ms.mt_threshold = msort_mt_threshold;
    ms.parity = 0;
    ms.level = thread_depth;
    msort_mt_main<false>(&ms, lessthan);
}

template <typename Iterator>
void msort_mt_mv(Iterator work, Iterator left, ptrdiff_t n,
		 int thread_depth,
		 int msort_mt_threshold = C7_MSORT_MT_THRESHOLD,
		 int msort_threshold = C7_MSORT_THRESHOLD)
{
    msort_mt_mv(work,
		left, n,
		[](const decltype(*left)& p,
		   const decltype(*left)& q) { return p < q; },
		thread_depth, msort_mt_threshold, msort_threshold);
}


/*----------------------------------------------------------------------------
                          quick sort - single thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Lessthan>
static Iterator qsort_partition_copy(Iterator left, Iterator right, Lessthan lessthan)
{
    std::remove_reference_t<decltype(*left)> s;
    {
	ptrdiff_t n = right - left;
	auto& v0 = left [     1];
	auto& v1 = left [n >> 1];
	auto& v2 = right[    -1];
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
	    using std::swap;
	    swap(*p, *q);
	    p++;
	    q--;
	}
    } while (p <= q);
    return p;
}

template <typename Iterator, typename Lessthan>
static Iterator qsort_partition_move(Iterator left, Iterator right, Lessthan lessthan)
{
    using std::swap;

    Iterator sp;
    {
	ptrdiff_t n = right - left;
	auto v0 = left  + 1;
	auto v1 = left  + (n >> 1);
	auto v2 = right - 1;
	if (lessthan(*v0, *v1)) {
	    if (lessthan(*v1, *v2))
		sp = v1;
	    else if (lessthan(*v0, *v2))
		sp = v2;
	    else
		sp = v0;
	} else {
	    if (lessthan(*v0, *v2))
		sp = v0;
	    else if (lessthan(*v1, *v2))
		sp = v2;
	    else
		sp = v1;
	}
	if (sp != v1) {
	    swap(*sp, *v1);
	    sp = v1;
	}
    }

    Iterator p = left;
    Iterator q = right;

    while (lessthan(*p, *sp))
	p++;
    while (lessthan(*sp, *q))
	q--;
    if (sp == p) {
	sp = q;
    } else if (sp == q) {
	sp = p;
    } else {
	swap(*p, *sp);
	sp = q;
    }
    swap(*p, *q);
    p++;
    q--;

    while (p <= q) {
	while (lessthan(*p, *sp))
	    p++;
	while (lessthan(*sp, *q))
	    q--;
	if (p <= q) {
	    swap(*p, *q);
	    p++;
	    q--;
	}
    }
    return p;
}

template <bool StdPartition, typename Iterator, typename Lessthan>
static void qsort_st_main(Iterator left, Iterator right,
			  Lessthan lessthan, int qsort_threshold)
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

	Iterator p;
	if constexpr (StdPartition) {
	    p = qsort_partition_copy(left, right, lessthan);
	} else {
	    p = qsort_partition_move(left, right, lessthan);
	}

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
	      int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_st_main<true>(left, (left + n) -1,
			lessthan,
			qsort_threshold);
}

template <typename Iterator>
void qsort_st(Iterator left, ptrdiff_t n,
	      int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_st_main<true>(left, (left + n) -1,
			[](const decltype(*left)& p,
			   const decltype(*left)& q) { return p < q; },
			qsort_threshold);
}

template <typename Iterator, typename Lessthan>
void qsort_st_mv(Iterator left, ptrdiff_t n, Lessthan lessthan,
		 int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_st_main<false>(left, (left + n) -1,
			 lessthan,
			 qsort_threshold);
}

template <typename Iterator>
void qsort_st_mv(Iterator left, ptrdiff_t n,
		 int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_st_main<false>(left, (left + n) -1,
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
    ptrdiff_t mt_threshold;
    int level;
};

template <bool StdPartition, typename Iterator, typename Lessthan>
static void qsort_mt_main(const qsort_param_t<Iterator>* const qs,
			  Lessthan lessthan)
{
    ptrdiff_t n = qs->right - qs->left;
    if (qs->level == 0 || n < qs->mt_threshold) {
	qsort_st_main<StdPartition>(qs->left, qs->right, lessthan, qs->threshold);
	return;
    }

    // right - left >= C7_QSORT_THRESHOLD

    Iterator p;
    if constexpr (StdPartition) {
	p = qsort_partition_copy(qs->left, qs->right, lessthan);
    } else {
	p = qsort_partition_move(qs->left, qs->right, lessthan);
    }

    std::thread th;
    qsort_param_t<Iterator> qs1, qs2;

    if (qs->left < (p - 1)) {
	qs1.left = qs->left;
	qs1.right = p - 1;
	qs1.threshold = qs->threshold;
	qs1.mt_threshold = qs->mt_threshold;
	qs1.level = qs->level - 1;
	th = std::thread(qsort_mt_main<StdPartition, Iterator, Lessthan>, &qs1, lessthan);
    }
    if (p < qs->right) {
	qs2.left = p;
	qs2.right = qs->right;
	qs2.threshold = qs->threshold;
	qs2.mt_threshold = qs->mt_threshold;
	qs2.level = qs->level - 1;
	qsort_mt_main<StdPartition>(&qs2, lessthan);
    }
    if (qs->left < (p - 1)) {
	th.join();
    }
}

template <typename Iterator, typename Lessthan>
void qsort_mt(Iterator left, ptrdiff_t n, Lessthan lessthan,
	      int thread_depth,
	      int qsort_mt_threshold = C7_QSORT_MT_THRESHOLD,
	      int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_param_t<Iterator> qs;
    qs.left = left;
    qs.right = left + (n - 1);
    qs.threshold = qsort_threshold;
    qs.mt_threshold = qsort_mt_threshold;
    qs.level = thread_depth;
    qsort_mt_main<true>(&qs, lessthan);
}

template <typename Iterator>
void qsort_mt(Iterator left, ptrdiff_t n,
	      int thread_depth,
	      int qsort_mt_threshold = C7_QSORT_MT_THRESHOLD,
	      int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_mt(left, n,
	     [](const decltype(*left)& p,
		const decltype(*left)& q) { return p < q; },
	     thread_depth, qsort_mt_threshold, qsort_threshold);
}

template <typename Iterator, typename Lessthan>
void qsort_mt_mv(Iterator left, ptrdiff_t n, Lessthan lessthan,
		 int thread_depth,
		 int qsort_mt_threshold = C7_QSORT_MT_THRESHOLD,
		 int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_param_t<Iterator> qs;
    qs.left = left;
    qs.right = left + (n - 1);
    qs.threshold = qsort_threshold;
    qs.mt_threshold = qsort_mt_threshold;
    qs.level = thread_depth;
    qsort_mt_main<false>(&qs, lessthan);
}

template <typename Iterator>
void qsort_mt_mv(Iterator left, ptrdiff_t n,
		 int thread_depth,
		 int qsort_mt_threshold = C7_QSORT_MT_THRESHOLD,
		 int qsort_threshold = C7_QSORT_THRESHOLD)
{
    qsort_mt_mv(left, n,
		[](const decltype(*left)& p,
		   const decltype(*left)& q) { return p < q; },
		thread_depth, qsort_mt_threshold, qsort_threshold);
}


/*----------------------------------------------------------------------------
                          radix sort - single thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Masktype, typename Getkey>
static void rsort_st_main(Iterator left, Iterator right,
			  Masktype keymask, Masktype tstmask, Getkey getkey,
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

    auto lessthan = [&getkey, keymask](const decltype(*left)& p,
				       const decltype(*left)& q) {
	return ((getkey(p) & keymask) < (getkey(q) & keymask));
    };

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
	    using std::swap;
	    swap(*p, *q);
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
	      int rsort_threshold = C7_RSORT_THRESHOLD)
{
    Masktype tstmask = ((Masktype)1) << (sizeof(Masktype)*8 - 1);
    rsort_st_main(left, left + n -1, keymask, tstmask, getkey, rsort_threshold);
}


/*----------------------------------------------------------------------------
                          radix sort - multi thread
----------------------------------------------------------------------------*/

template <typename Iterator, typename Masktype>
struct rsort_prm_t {
    Iterator left;
    Iterator right;
    ptrdiff_t threshold;
    ptrdiff_t mt_threshold;
    Masktype keymask;
    Masktype tstmask;
    int level;
};

template <typename Iterator, typename Masktype, typename Getkey>
static void rsort_mt_main(const rsort_prm_t<Iterator, Masktype> *rs, Getkey getkey)
{
    Iterator p = rs->left;
    Iterator q = rs->right;
    Masktype keymask = rs->keymask;
    Masktype tstmask = rs->tstmask;

    if (rs->level == 0 || (q - p) < rs->mt_threshold) {
	rsort_st_main(p, q, keymask, tstmask, getkey, rs->threshold);
	return;
    }

    while (p <= q && (getkey(*p) & tstmask) == 0)
	p++;
    while (p <= q && (getkey(*q) & tstmask) != 0)
	q--;
    while (p < q) {
	using std::swap;
	swap(*p, *q);
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
	rs1.mt_threshold = rs->mt_threshold;
	rs1.level = rs->level - 1;
	rs1.keymask = keymask;
	rs1.tstmask = tstmask;
	th = std::thread(rsort_mt_main<Iterator, Masktype, Getkey>, &rs1, getkey);
    }
    if (p < rs->right) {
	rs2.left = p;
	rs2.right = rs->right;
	rs2.threshold = rs->threshold;
	rs2.mt_threshold = rs->mt_threshold;
	rs2.level = rs->level - 1;
	rs2.keymask = keymask;
	rs2.tstmask = tstmask;
	rsort_mt_main(&rs2, getkey);
    }
    if (rs->left < q)
	th.join();
}

template <typename Iterator, typename Masktype, typename Getkey>
void rsort_mt(Iterator left, ptrdiff_t n, Masktype keymask, Getkey getkey,
	      int thread_depth,
	      int rsort_mt_threshold = C7_RSORT_MT_THRESHOLD,
	      int rsort_threshold = C7_RSORT_THRESHOLD)
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
    rs.mt_threshold = rsort_mt_threshold;
    rs.level = thread_depth;
    rs.keymask = keymask;
    rs.tstmask = tstmask;
    rsort_mt_main(&rs, getkey);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7sort.hpp
