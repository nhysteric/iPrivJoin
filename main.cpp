
#include <libOTe/Tools/ExConvCode/ExConvCode.h>
#include <volePSI/Defines.h>
#include <volePSI/RsOpprf.h>
#include "client.h"
#include "context.h"
#include "lpn.h"
#include "server.h"

int main(int argc, char **argv)
{
    // std::vector<block> r(8);
    // volePSI::PRNG prng({ 123, 0 });
    // prng.get(r.data(),8);
    // auto r_ = encode(r);
    // for (auto &a : r_) {
    //     std::cout << a << " \n";
    // }
    // size_t role = std::atoi(argv[1]);
    std::string task_name = argv[1];
    std::string config_file = "test/config/" + task_name + ".toml";
    std::cout << config_file << std::endl;
    std::string output_filePA = "test/wan/" + task_name + "_PA.txt";
    std::string output_filePB = "test/wan/" + task_name + "_PB.txt";
    PsiAnalyticsContext contextPA(PA, config_file, "PA.csv", output_filePA);
    PsiAnalyticsContext contextPB(PB, config_file, "PB.csv", output_filePB);
    auto futurePA = std::async(client_run, std::ref(contextPA));
    auto futurePB = std::async(server_run, std::ref(contextPB));
    futurePA.get();
    // if (role == 0) {
    //     PsiAnalyticsContext context(PA, config_file, "PA.csv", output_filePA);
    //     client_run(context);
    // } else {
    //     PsiAnalyticsContext context(PB, config_file, "PB.csv", output_filePA);
    //     server_run(context);
    // }
    return 0;
    // osuCrypto::ExConvCode ecc;
    // ecc.config(1);
    // std::vector<block> b(2);

    // b[0] = block(10000, 200000);
    // b[1] = block(300000, 400000);
    // for (auto &a : b) {
    //     std::cout << a << std::endl;
    // }
    // ecc.dualEncode<block>(b);
    // int high = 15;
    // std::string address = "127.0.0.1:8800";
    // std::random_device rd;
    // std::mt19937_64 generator(rd());
    // std::uniform_int_distribution<uint64_t> distribution(1, 100);
    // std::vector<block> origin = generateRandomVector<block>(1 << (high - 1), 123);
    // auto pi = generateRandomPermutation(1 << (high - 1), 123);
    // auto futureX = std::async(oShuffleSender, std::ref(origin), 4, std::ref(address));
    // auto futureY = std::async(oShuffleReceiver, origin.size(), 4, std::ref(address));
    // futureX.get();
    // auto y = futureY.get();
}