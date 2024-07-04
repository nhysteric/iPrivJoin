#pragma once

#include <array>
#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <string>
#include <vector>

using GGMTreeNode = uint64_t;
using PRGSeed = uint64_t;

class GGMTree
{
public:
    std::vector<uint64_t> node;
    uint64_t high;

    GGMTree(const uint64_t &high);
    GGMTree(const uint64_t &high, const uint64_t key);
    GGMTree(const uint64_t &high, const std::vector<osuCrypto::block> &received, const std::string &choices);

    ~GGMTree() = default;

    void set(const uint64_t &index, const uint64_t key);
    uint64_t xorLeft(const uint64_t &layer) const;
    uint64_t xorRight(const uint64_t &layer) const;
    uint64_t xorLayer(const uint64_t &layer) const;

    std::vector<std::array<osuCrypto::block, 2>> eval() const;

    std::vector<uint64_t> leaf() const;

    void print() const;
};

void gen(const PRGSeed &in, PRGSeed &first, PRGSeed &second);
void genXOR(const PRGSeed &in, PRGSeed &first, PRGSeed &second);
void ggm_send(const GGMTree &g, const std::string &address);
GGMTree ggm_recv(const uint64_t &path_num, const uint64_t &high, const std::string &address);
void printV(const std::vector<uint64_t> &v);