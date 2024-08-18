#include <algorithm>
#include <cassert>
#include <coproto/Common/span.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <libOTe/Tools/Coproto.h>
#include <vector>
#include "client.h"
#include "constants.h"
#include "joinData.h"
#include "lpn.h"
#include "opprf.h"
#include "oprf.h"
#include "shuffle.h"
#include "utlis.h"
void pb_map(
    const std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> &map,
    const Matrix &data,
    const oc::Matrix<block> &value,
    std::vector<block> &key,
    oc::Matrix<block> &r2,
    const PsiAnalyticsContext &context)
{
    for (auto &[local, index_id] : map) {
        size_t i = local * context.max_in_bin;
        for (size_t j = 0; j < index_id.size(); j++) {
            key[i + j] = block(local, index_id[j].second);
            auto m = minus(data[index_id[j].first], value[local]);
            assert(m.size() == r2.cols());
            for (size_t k = 0; k < m.size(); k++) {
                r2(i + j, k) = m[k];
            }
        }
    }
}

oc::Matrix<block> pb_share(PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    oc::Matrix<block> outputs(context.bins, context.pa_features);
    const uint64_t maxChunkSize = std::numeric_limits<std::uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = outputs.size();
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        oc::span<block> outputsSpan(outputs.data() + offset, chunkSize);
        auto p = chl.recv(outputsSpan);
        coproto::sync_wait(p);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis total_time = end_time - start_time;
    context.timings.share = total_time.count();
    return outputs;
}

// 使用简单哈希的一方
void server_run(PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    const auto init_start_time = std::chrono::system_clock::now();
    joinData pb(context);
    const auto init_end_time = std::chrono::system_clock::now();
    const duration_millis init_time = init_end_time - init_start_time;
    context.timings.init = init_time.count();
    oc::PRNG prng(block(rand(), rand()));
    auto map = SimpleHash(pb.ids, context);
    std::cout << "finish simple hash\n";
    std::vector<block> key(context.bins * context.max_in_bin);
    std::vector<block> r1(context.bins * context.max_in_bin);
    std::vector<block> r1_(context.bins);
    oc::Matrix<block> r2(context.bins * context.max_in_bin, context.pb_features);
    prng.get(key.data(), context.bins * context.max_in_bin);
    prng.get(r2.data(), context.bins * context.max_in_bin * context.pb_features);
    oc::Matrix<block> _b(context.bins, context.pb_features);

    for (size_t local = 0; local < context.bins; local++) {
        block b(prng.get());
        std::vector<block> a(context.pb_features);
        prng.get(a.data(), context.pb_features);
        for (size_t i = 0; i < a.size(); i++) {
            _b(local, i) = a[i];
        }
        r1_[local] = b;
        std::fill(
            r1.begin() + local * context.max_in_bin,
            r1.begin() + (local + 1) * context.max_in_bin,
            b);
    }
    pb_map(map, pb.features, _b, key, r2, context);
    opprfSender_1(key, r1, context);
    opprfSender_2(key, r2, context);
    auto newID = oprfReceiver(r1_, context);
    auto _a = pb_share(context);
    auto pb_data = mergeMatrix(newID, _a, _b, context);
    if (context.use_ture_shuflle) {
        shuffle_sender(pb_data, context);
        auto [pa_data, p] = shuffle_receiver(context);
        permuteMatrix(pb_data, p);
        pb_data += pa_data;
    } else {
        // fake shuffle_sender
        const auto c1 = std::chrono::system_clock::now();
        std::vector<block> mask(context.fill_bins);
        // prng.get(mask.data(), context.fill_bins);
        pb_data += khprf(mask, context);
        pb_data.setZero();
        pb_data -= khprf(mask, context);
        const auto c2 = std::chrono::system_clock::now();

        // fake shuffle_receiver
        Matrix outputs(context.fill_bins, context.pb_features + context.pa_features + 1);
        auto p = generateRandomPermutation(context.fill_bins, context.seedJ);
        // prng.get(mask.data(), context.fill_bins);
        // prng.get(outputs.data(), outputs.size());
        outputs -= khprf(mask, context);
        permuteMatrix(pb_data, p);
        pb_data += outputs;
        const auto c3 = std::chrono::system_clock::now();
        context.timings.shuffle1st =
            std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();
        context.timings.shuffle2nd =
            std::chrono::duration_cast<std::chrono::milliseconds>(c3 - c2).count();
    }

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis total_time = end_time - start_time;
    context.timings.total = total_time.count();
    context.print();
}
