#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>
#include "joinData.h"
std::vector<uint64_t> generateRandomPermutation(const uint64_t &n, const int &seed);
template <typename T>
std::vector<T> generateRandomVector(const uint64_t &n, const uint64_t &seed);
template <typename T>
void permuteVector(std::vector<T> &permutation, const std::vector<uint64_t> &pi);
template <typename T>
void xorVectors(std::vector<T> &v1, const std::vector<T> &v2);
template <typename T>
void xorVectorsWithPI(
    std::vector<T> &v1, const std::vector<T> &v2, const std::vector<uint64_t> &pi);
std::vector<block> mShuffleReceiver(
    const uint64_t &h,
    std::vector<uint64_t> &pi,
    const std::string &address,
    PsiAnalyticsContext &context);
std::vector<block> mShuffleSender(
    const std::vector<block> &x,
    const uint64_t &h,
    const std::string &address,
    PsiAnalyticsContext &context);
void oShuffleSender(std::vector<block> &inputs, PsiAnalyticsContext &context);
std::pair<std::vector<block>, std::vector<uint64_t>> oShuffleReceiver(
    const size_t &nums,
    const uint64_t &layers,
    const std::string &address,
    PsiAnalyticsContext &context);
class OShullfe {
public:
    uint64_t layers;
    uint64_t nums;
    std::vector<std::vector<uint64_t>> pbox;
    std::vector<std::vector<std::vector<uint64_t>>> sbox;
    std::vector<uint64_t> permutation;
    OShullfe(uint64_t nums, uint64_t layers);
};
void shuffle_sender(Matrix &inputs, PsiAnalyticsContext &context);

std::pair<Matrix, std::vector<uint64_t>> shuffle_receiver(PsiAnalyticsContext &context);