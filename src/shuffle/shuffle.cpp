#include "shuffle.h"
#include <algorithm>
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iostream>
#include <libOTe/Tools/Coproto.h>
#include <numeric>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
#include "../threadPool.h"
#include "../ggm/ggm.h"
// 生成随机vector

std::string incrementPort(const std::string &address, std::uint8_t offset);
std::vector<uint64_t> generateRandomVector(const uint64_t &n, const int &seed)
{
    std::mt19937 rng(seed);
    std::vector<uint64_t> vector(n);
    for (int i = 0; i < n; ++i) {
        vector[i] = rng();
    }
    return vector;
}
// 生成从0到n-1的排列
std::vector<uint64_t> generateRandomPermutation(const uint64_t &n, const int &seed)
{
    std::mt19937 rng(seed);
    std::vector<uint64_t> permutation(n);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    return permutation;
}

// 将给定向量按照全排列向量洗牌全排列
void permuteVector(std::vector<uint64_t> &permutation, const std::vector<uint64_t> &pi)
{
    assert(permutation.size() == pi.size());
    std::vector<uint64_t> resultVector(permutation.size());
    std::transform(pi.begin(), pi.end(), resultVector.begin(), [&](uint64_t index) {
        return permutation[index];
    });
    permutation = std::move(resultVector);
}

// 将两个维数相等的向量逐元素异或，结果保存在第一个向量中
void xorVectors(std::vector<uint64_t> &v1, const std::vector<uint64_t> &v2)
{
    // 检查向量长度是否相同
    assert(v1.size() == v2.size());

    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[i];
    }
}

// 将两个维数相等的向量洗牌后逐元素异或，结果保存在第一个向量中
void xorVectorsWithPI(
    std::vector<uint64_t> &v1, const std::vector<uint64_t> &v2, const std::vector<uint64_t> pi)
{
    // 检查向量长度是否相同
    assert(v1.size() == v2.size());
    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[pi[i]];
    }
}

std::pair<uint64_t, std::vector<uint64_t>> mShuffleSend(
    const uint64_t &high, const uint64_t key, const std::string address)
{
    GGMTree g(high, key);
    ggm_send(g, address);
    return std::make_pair(g.xorLayer(high - 1), g.leaf());
}

std::vector<uint64_t> mShuffleSender(
    const std::vector<uint64_t> &x, const uint64_t &h, const std::string &address)
{
    uint64_t t = 1 << (h - 1);
    assert(x.size() == t);
    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<uint64_t> distribution;
    std::vector<uint64_t> b(t);
    std::vector<uint64_t> a(t, 0);
    std::vector<std::future<std::pair<uint64_t, std::vector<uint64_t>>>> results;
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    for (int i = 0; i < t; i++) {
        GGMTree g(h, distribution(generator));
        b.at(i) = g.xorLayer(h - 1);
        xorVectors(a, g.leaf());
        results.emplace_back(
            pool.enqueue(mShuffleSend, h, distribution(generator), incrementPort(address, i)));
    }
    for (int i = 0; i < t; i++) {
        auto p = results[i].get();
        b.at(i) = p.first;
        xorVectors(a, p.second);
    }

    auto socket = osuCrypto::cp::asioConnect(address, false);

    xorVectors(a, x);
    osuCrypto::cp::sync_wait(socket.send(a));
    osuCrypto::cp::sync_wait(socket.flush());
    socket.close();
    return b;
}

std::vector<uint64_t> mShuffleReceiver(
    const uint64_t &h, std::vector<uint64_t> &pi, const std::string &address)
{
    uint64_t t = 1 << (h - 1);
    std::vector<uint64_t> b(t);
    std::vector<uint64_t> a_pi(t, 0);
    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<uint64_t> distribution;
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    std::vector<std::future<GGMTree>> results;
    for (int i = 0; i < t; i++) {
        results.push_back(pool.enqueue(ggm_recv, pi[i], h, incrementPort(address, i)));
    }
    for (int i = 0; i < t; i++) {
        auto g = results[i].get();
        b.at(i) = g.xorLayer(h - 1);
        xorVectorsWithPI(a_pi, g.leaf(), pi);
    }
    xorVectors(a_pi, b);

    auto socket = osuCrypto::cp::asioConnect(address, true);
    std::vector<uint64_t> xXORa(t, 0);
    osuCrypto::cp::sync_wait(socket.recv(xXORa));
    osuCrypto::cp::sync_wait(socket.flush());
    socket.close();
    xorVectorsWithPI(a_pi, xXORa, pi);
    return a_pi;
}

// 对一层进行全排列
void sboxPermutation(std::vector<uint64_t> &p, const std::vector<std::vector<uint64_t>> sbox)
{
    assert(p.size() / 2 == sbox.size());
    auto t = p.begin();
    for (int i = 0; i < sbox.size(); i++) {
        if (sbox.at(i).at(0) != 0) {
            std::iter_swap(t, t + 1);
        }
        t += 2;
    }
}

std::string incrementPort(const std::string &address, std::uint8_t offset = 1)
{
    size_t colonPos = address.find(':');
    if (colonPos == std::string::npos) {
        return "";
    }
    std::string portStr = address.substr(colonPos + 1);
    int port = std::stoi(portStr);
    port += offset;
    std::ostringstream oss;
    oss << port;
    return address.substr(0, colonPos) + ":" + oss.str();
}

OShullfe::OShullfe(uint64_t nums, uint64_t layers) : nums(nums), layers(layers)
{
    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<uint64_t> distribution;

    uint64_t players = layers / 2;
    uint64_t slayers = layers - players;
    uint64_t suints = nums / 2;
    pbox.resize(players, std::vector<uint64_t>(nums, 0));
    sbox.resize(slayers, std::vector<std::vector<uint64_t>>(suints, std::vector<uint64_t>(2, 0)));
    permutation.resize(nums);

    for (int i = 0; i < nums; i++) {
        permutation.at(i) = i;
    }

    for (int i = 0; i < slayers; i++) {
        for (int j = 0; j < suints; j++) {
            auto temp = generateRandomPermutation(2, distribution(generator));
            std::copy(temp.begin(), temp.begin() + 2, sbox.at(i).at(j).begin());
        }
    }
    for (int i = 0; i < players; i++) {
        pbox.at(i) = generateRandomPermutation(nums, distribution(generator));
    }
    for (int i = 0; i < layers; i++) {
        if (i % 2 == 0) {
            sboxPermutation(permutation, sbox.at(i / 2));
        } else {
            permuteVector(permutation, pbox.at(i / 2));
        }
    }
}

std::pair<std::vector<uint64_t>, std::vector<uint64_t>> oShuffleReceiver(
    const size_t &nums, const uint64_t &layers, const std::string &address)
{
    OShullfe oshullfe(nums, layers);
    auto socket = osuCrypto::cp::asioConnect(address, false);
    std::vector<uint64_t> temp(nums, 0);
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    for (int i = 0; i < layers; i++) {
        if (i % 2 == 0) {
            sboxPermutation(temp, oshullfe.sbox.at(i / 2));
            for (int j = 0; j < nums / 2; j++) {
                auto tempT =
                    mShuffleReceiver(2, oshullfe.sbox.at(i / 2).at(j), incrementPort(address));
                temp.at(2 * j) ^= tempT.at(0);
                temp.at(2 * j + 1) ^= tempT.at(1);
            }
        } else {
            osuCrypto::cp::sync_wait(socket.send(oshullfe.pbox.at(i / 2)));
            permuteVector(temp, oshullfe.pbox.at(i / 2));
        }
    }
    socket.close();
    return std::make_pair(std::move(temp), std::move(oshullfe.permutation));
}

void oShuffleSender(
    std::vector<uint64_t> &inputs, const uint64_t &layers, const std::string &address)
{
    size_t nums = inputs.size();

    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<uint64_t> distribution;

    auto socket = osuCrypto::cp::asioConnect(address, true);

    std::vector<uint64_t> pbox(inputs.size());
    for (int i = 0; i < layers; i++) {
        if (i % 2 == 0) {
            for (int j = 0; j < nums / 2; j++) {
                auto tempT = mShuffleSender(
                    std::vector<uint64_t>(inputs.begin() + 2 * j, inputs.begin() + 2 * j + 2),
                    2,
                    incrementPort(address));
                inputs.at(2 * j) = tempT.at(0);
                inputs.at(2 * j + 1) = tempT.at(1);
            }
        } else {
            osuCrypto::cp::sync_wait(socket.recv(pbox));
            permuteVector(inputs, pbox);
        }
    }
    osuCrypto::cp::sync_wait(socket.flush());
    socket.close();
}