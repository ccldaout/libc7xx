#include <c7sort.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <random>

#define N_ELM	10000

struct elm_t {
    uint32_t key;
    uint32_t data;
    inline bool operator<(const elm_t& rhs) {
	return this->key < rhs.key;
    }
};

typedef std::vector<elm_t> elmvec_t;
typedef std::vector<elm_t>::iterator elmit_t;

static elmvec_t BaseData;
static elmvec_t TestData;
static elmvec_t Ref1Data;	// by std::sort()
static elmvec_t Ref2Data;	// by std::sort()
static elmvec_t MsortBuf;

static void make_data1(elmvec_t& vec)
{
    static int seed = 1224;
    std::mt19937 engine(seed++);
    std::uniform_int_distribution<uint32_t> dist(1, N_ELM);
    
    for (auto& e: vec) {
	e.key = dist(engine);
	e.data = ~e.key;
    }
    std::cout << "------------ make_data1 #" << vec.size() << "-----------" << std::endl;
}

static void make_data2(elmvec_t& vec)
{
    static int seed = 1224;
    std::mt19937 engine(seed++);
    std::uniform_int_distribution<uint32_t> dist(1, N_ELM/10);
    
    for (auto& e: vec) {
	e.key = dist(engine);
	e.data = ~e.key;
    }
    std::cout << "------------ make_data2 #" << vec.size() << "-----------" << std::endl;
}

static void make_data3(elmvec_t& vec)
{
    uint32_t k = 1;
    for (auto& e: vec) {
	e.key = k++;
	e.data = ~e.key;
    }
    std::cout << "------------ make_data3 #" << vec.size() << "-----------" << std::endl;
}

static void make_data4(elmvec_t& vec)
{
    uint32_t k = N_ELM;
    for (auto& e: vec) {
	e.key = k++;
	e.data = ~e.key;
    }
    std::cout << "------------ make_data4 #" << vec.size() << "-----------" << std::endl;
}

static void make_data5(elmvec_t& vec)
{
    for (auto& e: vec) {
	e.key = 10;
	e.data = ~e.key;
    }
    std::cout << "------------ make_data5 #" << vec.size() << "-----------" << std::endl;
}

static inline bool gtope(const elm_t& lhs, const elm_t& rhs)
{
    return lhs.key > rhs.key;
}

template <typename Sorter1, typename Sorter2>
static void testsort(const char *msg, Sorter1 sorter1, Sorter2 sorter2)
{
    std::cout << msg << " ... " << std::flush;

    TestData = BaseData;
    sorter1(TestData.begin(), TestData.size());
    if (std::memcmp(TestData.data(), Ref1Data.data(),
		    TestData.size() * sizeof(elm_t)) != 0) {
	std::cout << "ERROR (1)" << std::endl;
	return;
    }

    TestData = BaseData;
    sorter2(TestData.data(), TestData.size());
    if (std::memcmp(TestData.data(), Ref2Data.data(),
		    TestData.size() * sizeof(elm_t)) != 0) {
	std::cout << "ERROR (2)" << std::endl;
	return;
    }
    std::cout << "passed" << std::endl;
}

static void testround(void (*make_data)(elmvec_t&))
{
    BaseData.resize(N_ELM);
    make_data(BaseData);

    Ref1Data = BaseData;
    std::sort(Ref1Data.begin(), Ref1Data.end());

    Ref2Data = BaseData;
    std::sort(Ref2Data.begin(), Ref2Data.end(), gtope);

    MsortBuf.resize(BaseData.size());

#if 0
    testsort("dsort",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::dsort(left, left+n-1);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::dsort(left, left+n-1, gtope);
	     });
#endif

    testsort("hsort_st",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::hsort_st(left, n);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::hsort_st(left, n, gtope);
	     });

    testsort("msort_st",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::msort_st(MsortBuf.begin(), left, n);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::msort_st(MsortBuf.data(), left, n, gtope);
	     });

    testsort("msort_mt",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::msort_mt(MsortBuf.begin(), left, n, 3);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::msort_mt(MsortBuf.data(), left, n, gtope, 3);
	     });

    testsort("qsort_st",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::qsort_st(left, n);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::qsort_st(left, n, gtope);
	     });

    testsort("qsort_mt",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::qsort_mt(left, n, 3);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::qsort_mt(left, n, gtope, 3);
	     });

    testsort("rsort_st",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::rsort_st(left, n, (uint32_t)-1, [](elm_t& e) {return e.key;});
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::rsort_st(left, n, (uint32_t)-1, [](elm_t& e) {return ~e.key;});
	     });

    testsort("rsort_mt",
	     [](elmit_t left, ptrdiff_t n) {
		 c7::rsort_st(left, n, (uint32_t)-1, [](elm_t& e) {return e.key;}, 3);
	     },
	     [](elm_t* left, ptrdiff_t n) {
		 c7::rsort_st(left, n, (uint32_t)-1, [](elm_t& e) {return ~e.key;}, 3);
	     });
}

int main()
{
    void (*makers[])(elmvec_t&) = {
	make_data1,
	make_data1,
	make_data1,
	make_data2,
	make_data2,
	make_data2,
	make_data3,
	make_data4,
	make_data5,
    };
    for (auto m: makers) {
	testround(m);
    }
    return 0;
}
