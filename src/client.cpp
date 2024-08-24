#include "client.h"
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "constants.h"
#include "joinData.h"
#include "lpn.h"
#include "opprf.h"
#include "oprf.h"
#include "shuffle.h"
#include "utlis.h"
// 使用布谷鸟哈希的一方

Matrix pa_share(
    const Matrix &origin,
    PsiAnalyticsContext &context,
    std::map<uint64_t, std::pair<uint64_t, uint64_t>> map)
{
    const auto start_time = std::chrono::system_clock::now();
    oc::PRNG prng(block(rand(), rand()));
    Matrix share(context.bins, context.pa_features);
    prng.get(share.data(), share.size());
    MatrixSend(share, context);
    for (size_t i = 0; i < context.bins; i++) {
        auto it = map.find(i);
        if (it != map.end()) {
            auto k = it->second.first;
            for (size_t j = 0; j < context.pa_features; j++) {
                share(i, j) = origin(k, j) - share(i, j);
            }
        }
    }
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis total_time = end_time - start_time;
    context.timings.share = total_time.count();
    return share;
}

void client_run(PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    const auto init_start_time = std::chrono::system_clock::now();

    joinData pa(context);

    const auto init_end_time = std::chrono::system_clock::now();
    const duration_millis init_time = init_end_time - init_start_time;
    context.timings.init = init_time.count();
    oc::PRNG prng(block(3, 4));
    auto map = CuckooHash(pa.ids, context);
    std::cout << "finish cuckoo hash\n";
    std::vector<block> key(context.bins);
    prng.get(key.data(), context.bins);
    for (const auto &[l, id] : map) {
        key[l] = block(l, id.first);
    }
    Matrix pa_data;
    {
        auto r1 = opprfReceiver_1(key, context);
        std::cout << "finish opprf1\n";
        auto r2 = opprfReceiver_2(key, context);
        std::cout << "finish opprf2\n";
        std::vector<block>().swap(key);
        auto newID = oprfSender(r1, context);
        std::cout << "finish oprf\n";
        auto _a = pa_share(pa.features, context, map);
        std::cout << "finish share\n";
        pa_data = mergeMatrix(newID, _a, r2, context);
    }
    if (context.use_ture_shuflle) {
        auto [pa_data_, p] = shuffle_receiver(context);
        permuteMatrix(pa_data, p);
        matrixTransform(pa_data_, pa_data);
        shuffle_sender(pa_data_, context);
    } else {
        const auto c1 = std::chrono::system_clock::now();
        std::vector<block> mask(context.fill_bins);
        // prng.get(mask.data(), context.fill_bins);

        // fake shuffle_receiver
        Matrix outputs(context.fill_bins, context.pb_features + context.pa_features + 1);
        auto p = generateRandomPermutation(context.fill_bins, context.seedJ);
        // prng.get(mask.data(), context.fill_bins);
        // prng.get(outputs.data(), outputs.size());
        Matrix expand = khprf(mask, context);
        MatrixRecv(outputs, context);
        expand += outputs;
        permuteMatrix(expand, p);
        permuteMatrix(pa_data, p);
        pa_data += expand;
        const auto c2 = std::chrono::system_clock::now();
        std::cout << "finish shuffle1\n";

        // fake shuffle_sender
        Matrix expand2 = khprf(mask, context);
        pa_data += expand2;
        MatrixSend(pa_data, context);
        pa_data.setZero();
        pa_data -= expand2;
        const auto c3 = std::chrono::system_clock::now();
        context.timings.shuffle1st =
            std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();
        context.timings.shuffle2nd =
            std::chrono::duration_cast<std::chrono::milliseconds>(c3 - c2).count();
        std::cout << "finish shuffle2\n";
    }

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis total_time = end_time - start_time;
    context.timings.total = total_time.count();
    context.print();
}
