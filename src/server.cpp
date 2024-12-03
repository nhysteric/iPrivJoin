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
#include "context.h"
#include "joinData.h"
#include "lpn.h"
#include "opprf.h"
#include "oprf.h"
#include "shuffle.h"
#include "utlis.h"
void pb_map(
    const std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> &map,
    const Matrix &data,
    const Matrix &_b,
    const std::vector<block> &r1_,
    std::vector<block> &r1,
    std::vector<block> &key,
    oc::Matrix<block> &r2,
    const PsiAnalyticsContext &context)
{
    int index = 0;
    for (int i = 0; i < context.bins; i++) {
        auto it = map.find(i);
        if (it != map.end()) {
            auto &v = it->second;
            for (auto &[id_index, id] : v) {
                r1[index] = r1_[i];
                if (id_index != -1) {
                    key[index] = block(id, i);
                    for (size_t j = 0; j < context.pb_features; j++) {
                        r2(index, j) = data(id_index, j) - _b(i, j);
                    }
                } else {
                    // 随便塞点假值
                    key[index] = block(rand(), rand());
                    for (size_t j = 0; j < context.pb_features; j++) {
                        r2(index, j) = block(id, j);
                    }
                }
                index++;
            }
        }
    }
}

oc::Matrix<block> pb_share(PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    oc::Matrix<block> outputs(context.bins, context.pa_features);
    MatrixRecv(outputs, context);
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
    std::vector<block> key(context.pb_elems * context.funcs);
    std::vector<block> r1(context.pb_elems * context.funcs);
    // OPRF 的 KEY
    std::vector<block> r1_(context.bins);
    oc::Matrix<block> _b(context.bins, context.pb_features);
    prng.get(r1_.data(), context.bins);
    prng.get(_b.data(), context.bins * context.pb_features);
    {
        oc::Matrix<block> r2(context.pb_elems * context.funcs, context.pb_features);
        prng.get(r2.data(), context.pb_elems * context.funcs * context.pb_features);
        pb_map(map, pb.features, _b, r1_, r1, key, r2, context);
        opprfSender_1(key, r1, context);
        opprfSender_2(key, r2, context);
    }
    std::vector<block>().swap(key);
    std::vector<block>().swap(r1);
    auto newID = oprfReceiver(r1_, context);
    Matrix pb_data;
    auto _a = pb_share(context);
    pb_data = mergeMatrix(newID, _a, _b, context);
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
        Matrix expand = khprf(mask, context);
        pb_data += expand;
        MatrixSend(pb_data, context);
        pb_data.setZero();
        pb_data -= expand;
        const auto c2 = std::chrono::system_clock::now();

        auto p = generateRandomPermutation(context.fill_bins, context.seedJ);
        Matrix expand2 = khprf(mask, context);
        {
            Matrix outputs(context.fill_bins, context.pb_features + context.pa_features + 1);
            // prng.get(mask.data(), context.fill_bins);
            // prng.get(outputs.data(), outputs.size());
            MatrixRecv(outputs, context);
            expand2 += outputs;
        }
        permuteMatrix(expand2, p);
        permuteMatrix(pb_data, p);
        pb_data += expand2;
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
