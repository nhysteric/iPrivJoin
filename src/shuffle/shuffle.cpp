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
#include <volePSI/Defines.h>
#include "../ggm/ggm.h"
#include "../threadPool.h"
#include "context.h"
#include "joinData.h"
#include "lpn.h"
#include "utlis.h"
// 生成随机vector

std::string incrementPort(const std::string &address, std::uint8_t offset);

template <typename T>
std::vector<T> generateRandomVector(const uint64_t &n, const uint64_t &seed)
{
    volePSI::PRNG rng({ seed, 0 });
    std::vector<T> vector(n);
    rng.get<T>(vector.data(), n);
    return vector;
}

template <>
std::vector<block> generateRandomVector(const uint64_t &n, const uint64_t &seed)
{
    volePSI::PRNG rng({ seed, 0 });
    std::vector<block> vector(n);
    rng.get<block>(vector.data(), n);
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

template <typename T>
void permuteVector(std::vector<T> &permutation, const std::vector<uint64_t> &pi)
{
    assert(permutation.size() == pi.size());
    std::vector<T> resultVector(permutation.size());
    std::transform(pi.begin(), pi.end(), resultVector.begin(), [&](uint64_t index) {
        return permutation[index];
    });
    permutation = std::move(resultVector);
}

template <typename T>
void xorVectors(std::vector<T> &v1, const std::vector<T> &v2)
{
    assert(v1.size() == v2.size());

    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[i];
    }
}

// 将两个维数相等的向量洗牌后逐元素异或，结果保存在第一个向量中
template <typename T>
void xorVectorsWithPI(std::vector<T> &v1, const std::vector<T> &v2, const std::vector<uint64_t> &pi)
{
    // 检查向量长度是否相同
    assert(v1.size() == v2.size());
    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[pi[i]];
    }
}

std::pair<block, std::vector<block>> mShuffleSend(
    const uint64_t &high, const block key, const std::string address, PsiAnalyticsContext &context)
{
    GGMTree g(high, key);
    ggm_send(g, address, context);
    return std::make_pair(g.xorLayer(high - 1), g.leaf());
}

std::vector<block> mShuffleSender(
    const std::vector<block> &x,
    const uint64_t &h,
    const std::string &address,
    PsiAnalyticsContext &context)
{
    uint64_t t = 1 << (h - 1);
    assert(x.size() == t);
    volePSI::PRNG prng({ 0, 0 });
    std::vector<block> b(t);
    std::vector<block> a(t, { 0, 0 });
    std::vector<std::future<std::pair<block, std::vector<block>>>> results;
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    for (int i = 0; i < t; i++) {
        GGMTree g(h, prng.get());
        b.at(i) = g.xorLayer(h - 1);
        xorVectors(a, g.leaf());
        results.emplace_back(
            pool.enqueue(mShuffleSend, h, prng.get(), incrementPort(address, i), context));
    }
    for (int i = 0; i < t; i++) {
        auto p = results[i].get();
        b.at(i) = p.first;
        xorVectors(a, p.second);
    }

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    xorVectors(a, x);
    osuCrypto::cp::sync_wait(chl.send(a));
    osuCrypto::cp::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    return b;
}

std::vector<block> mShuffleReceiver(
    const uint64_t &h,
    std::vector<uint64_t> &pi,
    const std::string &address,
    PsiAnalyticsContext &context)
{
    uint64_t t = 1 << (h - 1);
    std::vector<block> b(t);
    std::vector<block> a_pi(t, { 0, 0 });
    volePSI::PRNG prng({ 0, 0 });
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    std::vector<std::future<GGMTree>> results;
    for (int i = 0; i < t; i++) {
        results.push_back(pool.enqueue(ggm_recv, pi[i], h, incrementPort(address, i), context));
    }
    for (int i = 0; i < t; i++) {
        auto g = results[i].get();
        b.at(i) = g.xorLayer(h - 1);
        xorVectorsWithPI(a_pi, g.leaf(), pi);
    }
    xorVectors(a_pi, b);

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    std::vector<block> xXORa(t, { 0, 0 });
    osuCrypto::cp::sync_wait(chl.recv(xXORa));
    osuCrypto::cp::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    xorVectorsWithPI(a_pi, xXORa, pi);
    return a_pi;
}

// 对一层进行全排列
template <typename T>
void sboxPermutation(std::vector<T> &p, const std::vector<std::vector<uint64_t>> sbox)
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

OShullfe::OShullfe(uint64_t nums, uint64_t layers) : layers(layers), nums(nums)
{
    volePSI::PRNG prng({ 0, 0 });

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
            auto temp = generateRandomPermutation(2, prng.get());
            std::copy(temp.begin(), temp.begin() + 2, sbox.at(i).at(j).begin());
        }
    }
    for (int i = 0; i < players; i++) {
        pbox.at(i) = generateRandomPermutation(nums, prng.get());
    }
    for (int i = 0; i < layers; i++) {
        if (i % 2 == 0) {
            sboxPermutation(permutation, sbox.at(i / 2));
        } else {
            permuteVector(permutation, pbox.at(i / 2));
        }
    }
}

std::pair<std::vector<block>, std::vector<uint64_t>> oShuffleReceiver(
    const size_t &nums,
    const uint64_t &layers,
    const std::string &address,
    PsiAnalyticsContext &context)
{
    OShullfe oshullfe(nums, layers);
    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    std::vector<block> temp(nums, { 0, 0 });
    ThreadPool pool(std::thread::hardware_concurrency() / 2);
    for (int i = 0; i < layers; i++) {
        if (i % 2 == 0) {
            sboxPermutation(temp, oshullfe.sbox.at(i / 2));
            for (int j = 0; j < nums / 2; j++) {
                auto tempT = mShuffleReceiver(
                    2, oshullfe.sbox.at(i / 2).at(j), incrementPort(address), context);
                temp.at(2 * j) ^= tempT.at(0);
                temp.at(2 * j + 1) ^= tempT.at(1);
            }
        } else {
            osuCrypto::cp::sync_wait(chl.send(oshullfe.pbox.at(i / 2)));
            coproto::sync_wait(chl.flush());
            permuteVector(temp, oshullfe.pbox.at(i / 2));
        }
    }
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    return std::make_pair(std::move(temp), std::move(oshullfe.permutation));
}

void oShuffleSender(std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    size_t nums = inputs.size();

    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<uint64_t> distribution;

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    std::vector<uint64_t> pbox(inputs.size());
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            for (int j = 0; j < nums / 2; j++) {
                auto tempT = mShuffleSender(
                    std::vector<block>(inputs.begin() + 2 * j, inputs.begin() + 2 * j + 2),
                    2,
                    incrementPort(context.address),
                    context);
                inputs.at(2 * j) = tempT.at(0);
                inputs.at(2 * j + 1) = tempT.at(1);
            }
        } else {
            osuCrypto::cp::sync_wait(chl.recv(pbox));
            osuCrypto::cp::sync_wait(chl.flush());
            permuteVector(inputs, pbox);
        }
    }
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
}

void shuffle_sender(Matrix &inputs, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    oc::PRNG prng(block(rand(), rand()));
    std::vector<block> mask(context.fill_bins);
    prng.get(mask.data(), context.fill_bins);
    auto mask_ = mask;
    oShuffleSender(mask, context);
    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    inputs += khprf(mask_, context);
    osuCrypto::cp::sync_wait(chl.send(inputs));
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    inputs.setZero();
    inputs -= khprf(mask, context);
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis shuffle_time = end_time - start_time;
    switch (context.role) {
    case PA:
        context.timings.shuffle2nd = shuffle_time.count();
        break;
    case PB:
        context.timings.shuffle1st = shuffle_time.count();
        break;
    }
}

std::pair<Matrix, std::vector<uint64_t>> shuffle_receiver(PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    Matrix outputs(context.fill_bins, context.pb_features + context.pa_features + 1);
    auto [mask, p] = oShuffleReceiver(context.fill_bins, context.layers, context.address, context);
    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    osuCrypto::cp::sync_wait(chl.recv(outputs));
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();

    // permuteMatrix(outputs, p);
    outputs -= khprf(mask, context);
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis shuffle_time = end_time - start_time;
    switch (context.role) {
    case PA:
        context.timings.shuffle1st = shuffle_time.count();
        break;
    case PB:
        context.timings.shuffle2nd = shuffle_time.count();
        break;
    }
    return std::make_pair(outputs, p);
}