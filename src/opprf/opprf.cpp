#include "opprf.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
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
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    auto p = sender.send(context.bins, key, value, prng, 1, chl);
    coproto::sync_wait(p);
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf1_time = end_time - start_time;
    context.timings.opprf1st = opprf1_time.count();
}

std::vector<block> opprfReceiver_1(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    std::vector<block> outputs(context.bins);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));
    auto p = receiver.receive(context.bins * context.max_in_bin, key, outputs, prng, 1, chl);
    coproto::sync_wait(p);
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();

    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf1_time = end_time - start_time;
    context.timings.opprf1st = opprf1_time.count();

    return outputs;
}

void opprfSender_2(
    const std::vector<block> &key, oc::Matrix<block> &value, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, true);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();

    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    auto p = sender.send(context.bins, key, value, prng, 1, chl);
    coproto::sync_wait(p);
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf2_time = end_time - start_time;
    context.timings.opprf2nd = opprf2_time.count();
}

oc::Matrix<block> opprfReceiver_2(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    const auto start_time = std::chrono::system_clock::now();

    const auto wait_start_time = std::chrono::system_clock::now();
    coproto::Socket chl = coproto::asioConnect(context.address, false);
    const auto wait_end_time = std::chrono::system_clock::now();
    const duration_millis wait_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_time.count();
    oc::Matrix<block> outputs(context.bins, context.pb_features);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));

    auto p = receiver.receive(context.bins * context.max_in_bin, key, outputs, prng, 1, chl);
    coproto::sync_wait(p);
    coproto::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    chl.close();
    const auto end_time = std::chrono::system_clock::now();
    const duration_millis opprf2_time = end_time - start_time;
    context.timings.opprf2nd = opprf2_time.count();

    return outputs;
}
