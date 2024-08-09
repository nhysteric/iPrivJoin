#include <libOTe/Tools/ExConvCode/ExConvCode.h>
#include "joinData.h"
std::vector<osuCrypto::ExConvCode> generateConvCode(
    const uint64_t &messageSize, const uint64_t &loops);
Matrix encode(std::vector<block> &r, PsiAnalyticsContext &context);
Matrix khprf(const std::vector<block> &r, PsiAnalyticsContext &context);