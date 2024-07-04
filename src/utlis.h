#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>
#include "context.h"
void PrintInfo(const PsiAnalyticsContext &context);
std::vector<uint64_t> GetHashTableThroughFile(const std::string filePath);
std::vector<uint8_t> recv_vector(PsiAnalyticsContext &context, size_t size);
void send_vector(std::vector<uint64_t> &data, PsiAnalyticsContext &context);
std::map<uint64_t, uint64_t> CuckooHash(
    const std::vector<uint64_t> &ids, const PsiAnalyticsContext &context);