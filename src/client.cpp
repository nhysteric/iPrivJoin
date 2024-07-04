
// #include "client.h"
// #include <chrono>
// #include <cryptoTools/Common/Defines.h>
// #include <string>
// #include "constants.h"
// #include "shuffle.h"
// #include "utlis.h"
// // 使用布谷鸟哈希的一方
// void client_run(
//     const std::string inputFile, const std::string outputFIle, PsiAnalyticsContext &context)
// {
//     const auto start_time = std::chrono::system_clock::now();
//     std::vector<uint64_t> bins;
//     auto inputs = GetHashTableThroughFile(inputFile);

//     // 第一轮，作为接收方
//     bins = opprf_receiver(inputs, context);

//     // 第二轮，作为发送方

//     const auto oprf_start_time = std::chrono::system_clock::now();
//     bins = oprf_sender(bins, context);
//     const auto oprf_end_time = std::chrono::system_clock::now();
//     const duration_millis oprf_duration = oprf_end_time - oprf_start_time;
//     context.timings.oprf2nd = oprf_duration.count();
//     // TODO:执行两轮shuffle，分别作为发送方与接收方

//     std::string address = context.address + ":" + std::to_string(context.port);

//     const auto shuffle1st_start = std::chrono::system_clock::now();
//     // bins = mShuffleSender(bins, osuCrypto::log2ceil(bins.size()), address);
//     const auto shuffle1st_end = std::chrono::system_clock::now();
//     const duration_millis shuffle1st = shuffle1st_end - shuffle1st_start;
//     context.timings.shuffle1st = shuffle1st.count();

//     auto pi = generateRandomPermutation(osuCrypto::log2ceil(bins.size()), 123);
//     const auto shuffle2nd_start = std::chrono::system_clock::now();
//     bins = mShuffleReceiver(osuCrypto::log2ceil(bins.size()), pi, address);
//     const auto shuffle2nd_end = std::chrono::system_clock::now();
//     const duration_millis shuffle2nd = shuffle2nd_end - shuffle2nd_start;
//     context.timings.shuffle2nd = shuffle2nd.count();

//     const auto end_time = std::chrono::system_clock::now();
//     const duration_millis total_duration = end_time - start_time;
//     context.timings.total = total_duration.count();

//     // std::ofstream fout(outputFIle);
//     // for (auto value : bins)
//     // {
//     //     fout << static_cast<uint64_t>(value) << "\n";
//     // }
// }
