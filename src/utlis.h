#include <arpa/inet.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>
#include "context.h"
#include "joinData.h"
using block = osuCrypto::block;
void PrintInfo(const PsiAnalyticsContext &context);
std::vector<uint64_t> GetHashTableThroughFile(const std::string filePath);
std::vector<uint8_t> recv_vector(PsiAnalyticsContext &context, size_t size);
void send_vector(std::vector<uint64_t> &data, PsiAnalyticsContext &context);
std::map<uint64_t, std::pair<uint64_t, uint64_t>> CuckooHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context);
std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> SimpleHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context);
Matrix mergeMatrix(const std::vector<block> ids, const Matrix &a, const Matrix &b,PsiAnalyticsContext &context);
std::vector<block> flatten(const std::vector<std::vector<block>> &vec);
std::vector<std::vector<block>> unflatten(std::vector<block> &span, size_t inner_size);
std::vector<block> minus(const std::vector<block> &a, const oc::span<block> &b);
std::vector<block> minus(const std::vector<block> &a, const std::vector<block> &b);
std::vector<block> minus(const oc::span<block> &a, const oc::span<block> &b);
Matrix operator+(const Matrix &a, const Matrix &b);
void operator+=(Matrix &a, const Matrix &b);
void operator-=(Matrix &a, const Matrix &b);
void permuteMatrix(Matrix &a, const std::vector<size_t> &permute);
void matrixTransform(Matrix &a, const Matrix &b);
