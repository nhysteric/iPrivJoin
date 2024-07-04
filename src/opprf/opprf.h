#include <cryptoTools/Common/block.h>
#include <vector>

using block = osuCrypto::block;

void opprfSender(const std::vector<block> &key, std::vector<block> &value, std::string &address);

std::vector<block> opprfReceiver(const std::vector<block> &key, std::string &address);