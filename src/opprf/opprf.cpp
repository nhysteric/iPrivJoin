#include "opprf.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Network/Channel.h>
#include <string>
#include <vector>
#include <volePSI/RsOpprf.h>
void opprfSender(const std::vector<block> &key, std::vector<block> &value, std::string &address)
{
    coproto::Socket chl = coproto::asioConnect(address, true);
    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    auto p = sender.send(key.size(), key, value, prng, 1, chl);
    coproto::sync_wait(p);
    chl.close();
}

std::vector<block> opprfReceiver(const std::vector<block> &key, std::string &address)
{
    std::vector<block> value(key.size());
    coproto::Socket chl = coproto::asioConnect(address, false);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));
    auto p = receiver.receive(key.size(), key, value, prng, 1, chl);
    coproto::sync_wait(p);
    chl.close();
    return value;
}