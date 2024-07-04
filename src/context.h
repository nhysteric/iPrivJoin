#include <string>

#ifndef CONTEXT
#define CONTEXT
struct PsiAnalyticsContext {
    uint16_t port;
    uint64_t bitlen;
    uint64_t neles; // 元素数
    uint64_t nbins; // 桶数=neles*epsilon
    uint64_t notherpartyselems;
    uint64_t nthreads;
    uint64_t nfuns; //< number of hash functions in the hash table
    uint64_t threshold;
    uint64_t polynomialsize;
    uint64_t polynomialbytelength;
    uint64_t nmegabins; // 为了减轻计算多项式的插值开销，把nbins个桶合成nmegabins个桶
    double epsilon;
    std::string address;

    enum {
        NONE,      // only calculate the equality of the bin elements - used for benchmarking
        THRESHOLD, // 1 if T > PSI, 0 otherwise
        SUM,       // number of matched elements
        SUM_IF_GT_THRESHOLD // number of matched elements if T > PSI, 0 otherwise
    } analytics_type;

    const uint64_t maxbitlen = 61;

    struct {
        double hashing;
        double base_ots_libote;
        double oprf1st;
        double oprf2nd;
        double opprf;
        double polynomials;
        double polynomials_transmission;
        double total;
        double shuffle1st;
        double shuffle2nd;
        double wait = 0;
    } timings;
    unsigned long totalSend;
    unsigned long totalReceive;
};

#endif