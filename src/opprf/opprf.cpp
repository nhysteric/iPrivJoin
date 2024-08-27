#include "opprf.h"
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Network/Channel.h>
#include <vector>
#include <volePSI/RsOpprf.h>
#include "utlis.h"

void opprfSender_1(
    const std::vector<block> &key, std::vector<block> &value, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    const uint64_t maxBinSize = std::numeric_limits<std::uint32_t>::max() / 16 / context.max_in_bin;
    uint64_t offset = 0;
    uint64_t length = context.bins;

    while (length > 0) {
        uint64_t binSize = std::min(length, maxBinSize);
        oc::span<const block> keyspan(key.data() + offset, binSize * context.max_in_bin);
        oc::span<block> valuespan(value.data() + offset, binSize * context.max_in_bin);
        auto p = sender.send(binSize, keyspan, valuespan, prng, context.threads, chl);
        coproto::sync_wait(p);
        offset += binSize * context.max_in_bin;
        length -= binSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf1_time = end_time - start_time;
    context.timings.opprf1st = opprf1_time.count();
}

std::vector<block> opprfReceiver_1(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    std::vector<block> outputs(context.bins);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));
    const uint64_t maxBinSize = std::numeric_limits<std::uint32_t>::max() / 16 / context.max_in_bin;
    uint64_t offset = 0;
    uint64_t length = context.bins;

    while (length > 0) {
        uint64_t binSize = std::min(length, maxBinSize);
        oc::span<const block> keyspan(key.data() + offset, binSize);
        oc::span<block> outputspan(outputs.data() + offset, binSize);
        auto p = receiver.receive(
            binSize * context.max_in_bin, keyspan, outputspan, prng, context.threads, chl);
        coproto::sync_wait(p);
        offset += binSize;
        length -= binSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf1_time = end_time - start_time;
    context.timings.opprf1st = opprf1_time.count();

    return outputs;
}

void opprfSender_2(
    const std::vector<block> &key, oc::Matrix<block> &value, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_start_time = std::chrono::system_clock::now();

    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    const uint64_t maxBinSize =
        std::numeric_limits<std::uint32_t>::max() / 32 / context.max_in_bin / context.pb_features;
    uint64_t offset = 0;
    uint64_t length = context.bins;

    while (length > 0) {
        uint64_t binSize = std::min(length, maxBinSize);
        oc::span<const block> keyspan(key.data() + offset, binSize * context.max_in_bin);
        oc::MatrixView<block> valuespan(
            value.data() + offset * value.cols(), binSize * context.max_in_bin, value.cols());
        auto p = sender.send(binSize, keyspan, valuespan, prng, context.threads, chl);
        coproto::sync_wait(p);
        offset += binSize * context.max_in_bin;
        length -= binSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf2_time = end_time - start_time;
    context.timings.opprf2nd = opprf2_time.count();
}

oc::Matrix<block> opprfReceiver_2(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    oc::Matrix<block> outputs(context.bins, context.pb_features);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));

    const uint64_t maxBinSize =
        std::numeric_limits<std::uint32_t>::max() / 32 / context.max_in_bin / context.pb_features;

    uint64_t offset = 0;
    uint64_t length = context.bins;
    while (length > 0) {
        uint64_t binSize = std::min(length, maxBinSize);
        oc::span<const block> keyspan(key.data() + offset, binSize);
        oc::MatrixView<block> outputspan(
            outputs.data() + offset * outputs.cols(), binSize, outputs.cols());
        auto p = receiver.receive(
            binSize * context.max_in_bin, keyspan, outputspan, prng, context.threads, chl);
        coproto::sync_wait(p);

        offset += binSize;
        length -= binSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf2_time = end_time - start_time;
    context.timings.opprf2nd = opprf2_time.count();

    return outputs;
}
