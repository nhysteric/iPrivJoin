// #include "server.h"
// #include "constants.h"
// #include "opprf/opprf.h"
// #include "oprf/oprf.h"
// #include "utlis.h"
// // 使用简单哈希的一方
// void server_run(
//     const std::string inputFile, const std::string outputFIle, PsiAnalyticsContext &context)
// {
//     const auto start_time = std::chrono::system_clock::now();
//     std::vector<uint64_t> bins;
//     auto inputs = GetHashTableThroughFile(inputFile);
//     // 第一轮作为发送方
//     bins = opprf_sender(inputs, context);
//     // 第二轮作为接收方
//     const auto oprf_start_time = std::chrono::system_clock::now();
//     bins = oprf_receiver(bins, context);
//     const auto oprf_end_time = std::chrono::system_clock::now();
//     const duration_millis oprf_duration = oprf_end_time - oprf_start_time;
//     context.timings.oprf2nd = oprf_duration.count();

//     const auto end_time = std::chrono::system_clock::now();
//     const duration_millis total_duration = end_time - start_time;
//     context.timings.total = total_duration.count();

//     // TODO:执行两轮shuffle，分别作为接受方与发送方

//     // std::ofstream fout(outputFIle);
//     // for (auto value : bins)
//     // {
//     //     fout << static_cast<uint64_t>(value) << "\n";
//     // }
// }
