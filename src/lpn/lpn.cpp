#include "lpn.h"
#include <cassert>
#include <cryptoTools/Common/Defines.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "context.h"

Matrix encode(std::vector<block> &r, PsiAnalyticsContext &context)
{
    assert(r.size() == 2 * context.bins);
    size_t loops = context.pa_features + context.pb_features + 1;
    Matrix result(context.bins, loops);
    auto codes = generateConvCode(context.bins, loops);

    for (size_t i = 0; i < loops; i++) {
        auto t = r;
        std::vector<block> temp(context.bins);
        codes[i].dualEncode<block>(t, temp);
        for (size_t j = 0; j < context.bins; j++) {
            result(j, i) = temp[j];
        }
    }

    return result;
}

std::vector<osuCrypto::ExConvCode> generateConvCode(
    const uint64_t &messageSize, const uint64_t &loops)
{
    uint64_t codeSize = messageSize * 2, expanderWeight = 7, accumulatorSize = 16;
    bool systematic = false;
    std::vector<osuCrypto::ExConvCode> result(loops);
    for (size_t i = 0; i < loops; i++) {
        result[i].config(
            messageSize,
            codeSize,
            expanderWeight,
            accumulatorSize,
            systematic,
            { i + 100, i + 4123 });
    }
    return result;
}