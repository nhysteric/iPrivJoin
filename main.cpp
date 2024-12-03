
#include <libOTe/Tools/ExConvCode/ExConvCode.h>
#include <volePSI/Defines.h>
#include <volePSI/RsOpprf.h>
#include "client.h"
#include "context.h"
#include "lpn.h"
#include "server.h"

size_t getMemoryUsage()
{
    std::ifstream proc("/proc/self/status");
    std::string line;
    while (std::getline(proc, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            std::istringstream iss(line);
            std::string tmp;
            size_t memory;
            iss >> tmp >> memory;
            return memory; // in KB
        }
    }
    return 0;
}

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
    std::string env = argv[2];
    std::string config_file = "test/config/" + task_name + ".toml";
    std::cout << config_file << std::endl;
    std::string output_filePA = "test/" + env + "/" + task_name + "_PA.txt";
    std::string output_filePB = "test/" + env + "/" + task_name + "_PB.txt";
    PsiAnalyticsContext contextPA(PA, config_file, "PA.csv", output_filePA);
    PsiAnalyticsContext contextPB(PB, config_file, "PB.csv", output_filePB);
    auto futurePA = std::async(client_run, std::ref(contextPA));
    auto futurePB = std::async(server_run, std::ref(contextPB));
    std::vector<std::future<void>> futures;
    futures.push_back(std::move(futurePA));
    futures.push_back(std::move(futurePB));
    std::exception_ptr eptr = nullptr;
    while (!futures.empty()) {
        for (auto it = futures.begin(); it != futures.end();) {
            auto status = it->wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                try {
                    it->get(); // 获取结果或抛出异常
                } catch (...) {
                    eptr = std::current_exception(); // 捕获异常
                }
                it = futures.erase(it); // 移除已完成的future
            } else {
                ++it;
            }
        }
        if (eptr) {
            break; // 如果捕获了异常，退出循环
        }
    }
    // 处理捕获到的异常
    if (eptr) {
        try {
            std::rethrow_exception(eptr); // 重新抛出异常
        } catch (const std::exception &e) {
            std::cerr << "Exception caught: " << e.what() << std::endl;
        }
        std::terminate(); // 终止程序以避免进一步的死锁或其他问题
    }
    // if (role == 0) {
    //     PsiAnalyticsContext context(PA, config_file, "PA.csv", output_filePA);
    //     client_run(context);
    // } else {
    //     PsiAnalyticsContext context(PB, config_file, "PB.csv", output_filePA);
    //     server_run(context);
    // }
    // 不知道为什么不能正常退出
    // std::terminate();
    std::cout << "Peak memory usage: " << getMemoryUsage() << " KB\n";
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