#include <algorithm>
#include <cstddef>
#include <vector>
#include <volePSI/RsOpprf.h>
#include "opprf.h"
#include "oprf.h"
int main(int argc, char **argv)
{
    size_t n = 10;
    std::string address = "0.0.0.0:10011";
    size_t role = std::atoi(argv[1]);
    std::vector<block> result;
    if (role == 1) {
        oc::PRNG prng(block(0, role));
        std::vector<block> p(n);
        prng.get(p.data(), n);
        result = opprfReceiver(p, address);
    } else {
        oc::PRNG prng(block(0, role));
        std::vector<block> p(n);
        prng.get(p.data(), n);
        result.resize(n);
        prng.get(result.data(), n);
        opprfSender(p, result, address);
    }
    std::cout << role << "\n";
    std::for_each(result.begin(), result.end(), [](auto &v) { std::cout << v << "\n"; });
    // RsOprf_eval_test();
    std::cout << "end\n";
    return 0;
    std::cout << "end\n";
}