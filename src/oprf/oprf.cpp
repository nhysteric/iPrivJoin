#include "oprf.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Network/Channel.h>
#include <string>
#include <vector>
#include <volePSI/RsOprf.h>

std::vector<block> oprfSender(const std::vector<block> &inputs, std::string &address)
{
    coproto::Socket chl = coproto::asioConnect(address, true);
    oc::PRNG prng(block(0, 0));
    volePSI::RsOprfSender sender;
    auto p = sender.send(inputs.size(), prng, chl, 1);
    coproto::sync_wait(p);
    std::vector<oc::block> result(inputs.size());
    sender.eval(inputs, result);
    chl.close();
    return result;
}

std::vector<block> oprfReceiver(const std::vector<block> &inputs, std::string &address)
{
    std::vector<oc::block> result(inputs.size());
    coproto::Socket chl = coproto::asioConnect(address, false);
    oc::PRNG prng(oc::block(0, 0));
    volePSI::RsOprfReceiver receiver;
    auto p0 = receiver.receive(inputs, result, prng, chl, 1);
    coproto::sync_wait(p0);
    chl.close();
    return result;
}

inline auto eval(macoro::task<> &t0, macoro::task<> &t1)
{
    auto r = macoro::sync_wait(macoro::when_all_ready(std::move(t0), std::move(t1)));
    std::get<0>(r).result();
    std::get<1>(r).result();
}

void RsOprf_eval_test()
{
    using namespace volePSI;
    using namespace oc;
    using coproto::LocalAsyncSocket;
    RsOprfSender sender;
    RsOprfReceiver recver;

    auto sockets = LocalAsyncSocket::makePair();
    u64 n = 100;
    PRNG prng0(block(0, 0));
    PRNG prng1(block(0, 1));

    std::vector<block> vals(n), recvOut(n);

    prng0.get(vals.data(), n);

    auto p0 = sender.send(n, prng0, sockets[0]);
    auto p1 = recver.receive(vals, recvOut, prng1, sockets[1]);

    eval(p0, p1);
    std::cout << "end\n";
    std::vector<block> vv(n);
    sender.eval(vals, vv);
    prng0.get(vals.data(), n);
    for (u64 i = 0; i < n; ++i) {
        auto v = sender.eval(vals[i]);

        std::cout << i << " " << recvOut[i] << " " << v << " " << vv[i] << std::endl;
    }
}