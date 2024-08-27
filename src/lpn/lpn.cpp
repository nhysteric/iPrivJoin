#include "lpn.h"
#include <cassert>
#include <cryptoTools/Common/Defines.h>
#include <cstddef>
#include <vector>
#include "context.h"
#include "joinData.h"

Matrix khprf(const std::vector<block> &r, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    assert(r.size() == context.fill_bins);
    size_t loops = context.pa_features + context.pb_features + 1;
    Matrix result(context.fill_bins, loops);
    for (size_t i = 0; i < loops; i++) {
        for (size_t j = 0; j < context.fill_bins; j++) {
            result(j, i) = r[j] * context.J[j];
        }
    }
    const auto end_time = std::chrono::system_clock::now();
    context.timings.khprf += std::chrono::duration<double>(end_time - start_time).count();
    return result;
}
