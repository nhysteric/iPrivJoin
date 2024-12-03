#include "utlis.h"
#include <algorithm>
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/config.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <kuku/common.h>
#include <kuku/kuku.h>
#include <kuku/locfunc.h>
#include <libOTe/Tools/Coproto.h>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "constants.h"
#include "context.h"
#include "joinData.h"

uint64_t nextPowerOf2(uint64_t n)
{
    if (n == 0) {
        return 1;
    }

    if ((n & (n - 1)) == 0) {
        return n;
    }

    if (n >= (1ULL << 63))
        return 0;

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;

    return n;
}

void operator-=(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            a(i, j) -= b(i, j);
        }
    }
}

Matrix operator+(const Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    Matrix result(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            result(i, j) = a(i, j) + b(i, j);
        }
    }
    return result;
}

void operator+=(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            a(i, j) += b(i, j);
        }
    }
}

void matrixTransform(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        a(i, 0) -= b(i, 0);
        for (size_t j = 1; j < a.cols(); j++) {
            a(i, j) += b(i, j);
        }
    }
}

Matrix operator-(const Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    Matrix result(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            result(i, j) = a(i, j) - b(i, j);
        }
    }
    return result;
}

void permuteMatrix(Matrix &a, const std::vector<size_t> &permute)
{
    assert(a.rows() == permute.size());
    Matrix temp(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            temp(i, j) = a(permute[i], j);
        }
    }
    a = temp;
}

std::map<uint64_t, std::pair<uint64_t, uint64_t>> CuckooHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    kuku::KukuTable table(context.bins, 0, context.funcs, { 0, 0 }, 90000, kuku::make_item(0, 0));
    std::for_each(ids.begin(), ids.end(), [&table](const uint64_t &v) {
        if (!table.insert(kuku::make_item(v, 0))) {
            std::cout << "fill rate: " << table.fill_rate() << std::endl;
            std::cout << "failed item: " << v << std::endl;
            std::cout << "failed in kuku table" << std::endl;
            throw "failed in kuku table";
        }
    });
    std::cout << "kuku hash success with fill rate: " << table.fill_rate() << std::endl;
    std::map<uint64_t, std::pair<uint64_t, uint64_t>> loc_id_map;
    for (size_t i = 0; i < ids.size(); i++) {
        kuku::QueryResult res = table.query(kuku::make_item(ids[i], 0));
        loc_id_map[res.location()] = std::make_pair(i, ids[i]);
    }

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis hash_time = end_time - start_time;
    context.timings.hashing = hash_time.count();
    return loc_id_map;
}

std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> SimpleHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    oc::PRNG prng(block(context.seedJ - 1));
    std::vector<kuku::LocFunc> funcs;
    for (size_t i = 0; i < context.funcs; i++) {
        funcs.emplace_back(context.bins, kuku::make_item(i, 0));
    }
    // 分别代表：桶下标、id下标、id值
    std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> loc_index_id_map;

    for (size_t i = 0; i < ids.size(); i++) {
        std::set<uint64_t> sets = std::set<uint64_t>();
        for (auto f : funcs) {
            uint64_t loc = f(kuku::make_item(ids[i], 0));
            //! 偶尔会出现hash到一个桶里的情况，这回导致opprf失效，这里直接跳过
            // 现在不能直接跳过，转而生成一个随机元素放入桶中，并标注下标为-1
            if (!sets.insert(loc).second) {
                loc_index_id_map.at(loc).push_back(std::make_pair(-1, prng.get()));
                continue;
            }
            if (loc_index_id_map.find(loc) != loc_index_id_map.end()) {
                loc_index_id_map.at(loc).push_back(std::make_pair(i, ids[i]));
                // if (loc_index_id_map.at(loc).size() > context.max_in_bin) {
                //     std::cout << "max_in_bin with size" << loc_index_id_map.at(loc).size()
                //               << std::endl;
                //     throw "max_in_bin";
                // }
            } else {
                std::vector<std::pair<uint64_t, uint64_t>> temp;
                temp.push_back(std::make_pair(i, ids[i]));
                loc_index_id_map[loc] = temp;
            }
        }
    }
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis hash_time = end_time - start_time;
    context.timings.hashing = hash_time.count();
    return loc_index_id_map;
}
std::vector<block> minus(const std::vector<block> &a, const oc::span<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

std::vector<block> minus(const oc::span<block> &a, const oc::span<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

std::vector<block> minus(const std::vector<block> &a, const std::vector<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

Matrix mergeMatrix(
    const std::vector<block> ids, const Matrix &a, const Matrix &b, PsiAnalyticsContext &context)
{
    assert(a.rows() == b.rows() && a.rows() == ids.size());
    uint64_t fill_bins = nextPowerOf2(context.bins);
    context.fill_bins = fill_bins;
    context.J = std::vector<block>(fill_bins);
    oc::PRNG prng(block(context.seedJ));
    Matrix v(fill_bins, a.cols() + b.cols() + 1);
    prng.get(context.J.data(), fill_bins);
    for (size_t i = 0; i < a.rows(); i++) {
        v(i, 0) = ids[i];
        for (size_t j = 0; j < a.cols(); j++) {
            v(i, j + 1) = a(i, j);
        }
        for (size_t j = 0; j < b.cols(); j++) {
            v(i, j + a.cols() + 1) = b(i, j);
        }
    }
    return v;
}

void minus(std::vector<std::vector<block>> &a, const std::vector<std::vector<block>> &b)
{
    assert(a.size() == b.size());
    assert(a[0].size() == b[0].size());
    for (size_t i = 0; i < a.size(); i++) {
        for (size_t j = 0; j < a[i].size(); j++) {
            a[i][j] -= b[i][j];
        }
    }
}

void PrintInfo(const PsiAnalyticsContext &context)
{
    // std::cout << "Time for wait " << context.timings.wait << "ms\n";
    // std::cout << "Time for hashing " << context.timings.hashing << " ms\n";
    // std::cout << "Time for OPRF " << context.timings.oprf1st << " ms\n";
    // std::cout << "Time for polynomials " << context.timings.polynomials << " ms\n";
    // std::cout << "Time for transmission of the polynomials "
    //           << context.timings.polynomials_transmission << " ms\n";
    // std::cout << "Total runtime: " << context.timings.total << "ms\n";
    // std::cout << "Total runtime-wait time: " << context.timings.total - context.timings.wait
    //           << "ms\n";
    // std::cout << "Total receive: " << context.totalReceive * 1.0 / 1048576 << "MB\n";
    // std::cout << "Total send: " << context.totalSend * 1.0 / 1048576 << "MB\n";
}

void MatrixRecv(Matrix &result, PsiAnalyticsContext &context)
{
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = result.size();
    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl;
    while (true) {
        try {
            chl = coproto::asioConnect(context.address, true);
            std::cout << "connect success\n";
            break;
        } catch (std::exception e) {
            std::cout << "connect failed, retrying\n";
        }
    }

    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        oc::span<block> resultSpan(result.data() + offset, chunkSize);
        auto p = chl.recv(resultSpan);
        coproto::sync_wait(p);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
}

void MatrixSend(const Matrix &value, PsiAnalyticsContext &context)
{
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = value.size();
    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        osuCrypto::span<block> shareSpan(value.data() + offset, chunkSize);
        auto p = chl.send(shareSpan);
        coproto::sync_wait(p);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
}