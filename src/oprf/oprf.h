#include <cryptoTools/Common/block.h>
#include <vector>

using block = osuCrypto::block;
std::vector<block> oprfSender(const std::vector<block> &inputs, std::string &address);
std::vector<block> oprfReceiver(const std::vector<block> &inputs, std::string &address);
void RsOprf_eval_test();