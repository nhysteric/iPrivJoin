#include "oprf.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Network/Channel.h>
#include <string>
#include <vector>
#include <volePSI/RsOprf.h>

std::vector<block> oprfSender(const std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    oc::PRNG prng(block(0, 0));
    volePSI::RsOprfSender sender;
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = context.bins;
    std::vector<oc::block> result(context.bins);
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        auto p = sender.send(chunkSize, prng, chl, context.threads);
        coproto::sync_wait(p);

        osuCrypto::span<const block> inputSpan(inputs.data() + offset, chunkSize);
        osuCrypto::span<block> resultSpan(result.data() + offset, chunkSize);
        sender.eval(inputSpan, resultSpan);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis oprf_time = end_time - start_time;
    context.timings.oprf = oprf_time.count();
    return result;
}

std::vector<block> oprfReceiver(const std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    std::vector<oc::block> result(context.bins);

    oc::PRNG prng(oc::block(0, 0));
    volePSI::RsOprfReceiver receiver;
    uint64_t offset = 0;
    uint64_t length = context.bins;
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        osuCrypto::span<const block> inputSpan(inputs.data() + offset, chunkSize);
        osuCrypto::span<block> resultSpan(result.data() + offset, chunkSize);
        auto p0 = receiver.receive(inputSpan, resultSpan, prng, chl, context.threads);
        coproto::sync_wait(p0);

        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis oprf_time = end_time - start_time;
    context.timings.oprf = oprf_time.count();
    return result;
}
