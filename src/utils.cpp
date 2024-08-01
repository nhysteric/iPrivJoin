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
#include <kuku/common.h>
#include <kuku/kuku.h>
#include <kuku/locfunc.h>
#include <libOTe/Tools/Coproto.h>
#include <map>
#include <utility>
#include <vector>
#include "constants.h"
#include "joinData.h"
#include "utlis.h"

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

void send_vector(std::vector<uint64_t> &data, PsiAnalyticsContext &context)
{
    const auto wait_start_time = std::chrono::system_clock::now();

    auto socket = osuCrypto::cp::asioConnect(context.address, false);

    const auto wait_end_time = std::chrono::system_clock::now();

    const duration_millis wait_duration_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_duration_time.count();
    socket.send(data);

    coproto::sync_wait(socket.flush());
    socket.close();
    context.totalReceive += socket.bytesReceived();
    context.totalSend += socket.bytesSent();
}

std::vector<uint8_t> recv_vector(PsiAnalyticsContext &context, size_t size)
{
    const auto wait_start_time = std::chrono::system_clock::now();

    auto socket = osuCrypto::cp::asioConnect(context.address, true);

    const auto wait_end_time = std::chrono::system_clock::now();

    const duration_millis wait_duration_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_duration_time.count();

    std::vector<uint8_t> data(size);
    socket.recv(data);

    coproto::sync_wait(socket.flush());
    socket.close();
    context.totalReceive += socket.bytesReceived();
    context.totalSend += socket.bytesSent();
    return data;
}

std::map<uint64_t, std::pair<uint64_t, uint64_t>> CuckooHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    kuku::KukuTable table(context.bins, 0, 2, kuku::make_random_item(), 3, kuku::make_item(0, 0));
    std::for_each(ids.begin(), ids.end(), [&table](const uint64_t &v) {
        if (!table.insert(kuku::make_item(v, 0))) {
            throw "failed in kuku table";
        }
    });
    std::map<uint64_t, std::pair<uint64_t, uint64_t>> loc_id_map;
    for (size_t i = 0; i < ids.size(); i++) {
        kuku::QueryResult res = table.query(kuku::make_item(ids[i], 0));
        loc_id_map[res.location()] = std::make_pair(i, ids[i]);
    }
    return loc_id_map;
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis hash_time = end_time - start_time;
    context.timings.hashing = hash_time.count();
}

std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> SimpleHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    std::vector<kuku::LocFunc> funcs;
    for (size_t i = 0; i < context.funcs; i++) {
        funcs.emplace_back(context.bins, kuku::make_item(i, 0));
    }
    std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> loc_index_id_map;

    for (size_t i = 0; i < ids.size(); i++) {
        for (auto f : funcs) {
            uint64_t loc = f(kuku::make_item(ids[i], 0));

            if (loc_index_id_map.find(loc) != loc_index_id_map.end()) {
                loc_index_id_map.at(loc).push_back(std::make_pair(i, ids[i]));
                if (loc_index_id_map.at(loc).size() > context.max_in_bin)
                    throw "max_in_bin";
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

Matrix mergeMatrix(const std::vector<block> ids, const Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.rows() == ids.size());
    Matrix v(a.rows(), a.cols() + b.cols() + 1);
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