#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>
std::vector<uint64_t> generateRandomPermutation(const uint64_t &n, const int &seed);
std::vector<uint64_t> generateRandomVector(const uint64_t &n, const int &seed);
void permuteVector(std::vector<uint64_t> &permutation, const std::vector<uint64_t> &pi);
void xorVectors(std::vector<uint64_t> &v1, const std::vector<uint64_t> &v2);
void xorVectorsWithPI(
    std::vector<uint64_t> &v1, const std::vector<uint64_t> &v2, const std::vector<uint64_t> pi);
std::vector<uint64_t> mShuffleReceiver(
    const uint64_t &h, std::vector<uint64_t> &pi, const std::string &address);
std::vector<uint64_t> mShuffleSender(
    const std::vector<uint64_t> &x, const uint64_t &h, const std::string &address);
void oShuffleSender(
    std::vector<uint64_t> &inputs, const uint64_t &layers, const std::string &address);
std::pair<std::vector<uint64_t>, std::vector<uint64_t>> oShuffleReceiver(
    const size_t &nums, const uint64_t &layers, const std::string &address);
class OShullfe {
public:
    uint64_t layers;
    uint64_t nums;
    std::vector<std::vector<uint64_t>> pbox;
    std::vector<std::vector<std::vector<uint64_t>>> sbox;
    std::vector<uint64_t> permutation;
    OShullfe(uint64_t nums, uint64_t layers);
};