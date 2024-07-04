#include <algorithm>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/config.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>
#include <cstddef>
#include <cstdint>
#include <cuckoo_hashing/cuckoo_hashing.h>
#include <fstream>
#include <kuku/common.h>
#include <kuku/kuku.h>
#include <kuku/locfunc.h>
#include <libOTe/Tools/Coproto.h>
#include <map>
#include <simple_hashing/simple_hashing.h>
#include <vector>
#include "constants.h"
#include "utlis.h"
void send_vector(std::vector<uint64_t> &data, PsiAnalyticsContext &context)
{
    const auto wait_start_time = std::chrono::system_clock::now();

    auto socket =
        osuCrypto::cp::asioConnect(context.address + ":" + std::to_string(context.port), false);

    const auto wait_end_time = std::chrono::system_clock::now();

    const duration_millis wait_duration_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_duration_time.count();
    socket.send(data);

    coproto::sync_wait(socket.flush());
    socket.close();
    context.totalReceive += socket.bytesReceived();
    context.totalSend += socket.bytesSent();
}

std::vector<uint8_t> recv_vector(PsiAnalyticsContext &context, size_t size)
{
    const auto wait_start_time = std::chrono::system_clock::now();

    auto socket =
        osuCrypto::cp::asioConnect(context.address + ":" + std::to_string(context.port), true);

    const auto wait_end_time = std::chrono::system_clock::now();

    const duration_millis wait_duration_time = wait_end_time - wait_start_time;
    context.timings.wait += wait_duration_time.count();

    std::vector<uint8_t> data(size);
    socket.recv(data);

    coproto::sync_wait(socket.flush());
    socket.close();
    context.totalReceive += socket.bytesReceived();
    context.totalSend += socket.bytesSent();
    return data;
}
std::vector<uint64_t> GetHashTableThroughFile(const std::string filePath)
{
    std::vector<uint64_t> hash;
    std::ifstream file1(filePath);
    if (file1.is_open()) {
        std::string line;
        while (std::getline(file1, line)) {
            unsigned long number = std::stoul(line);
            hash.push_back(number);
        }
        file1.close();
    } else {
        std::cout << "Failed to open " << filePath << std::endl;
        return hash;
    }
    for (auto &e : hash) {
        e = ENCRYPTO::HashingTable::ElementToHash(e) & __61_bit_mask;
    }
    return hash;
}

std::map<uint64_t, uint64_t> CuckooHash(
    const std::vector<uint64_t> &ids, const PsiAnalyticsContext &context)
{
    kuku::KukuTable table(context.nbins, 0, 2, kuku::make_random_item(), 3, kuku::make_item(0, 0));
    std::for_each(ids.begin(), ids.end(), [&table](const uint64_t &v) {
        if (!table.insert(kuku::make_item(v, 0))) {
            throw "failed in kuku table";
        }
    });
    std::map<uint64_t, uint64_t> id_loc_map;
    std::for_each(ids.begin(), ids.end(), [&table, &id_loc_map](const uint64_t &v) {
        kuku::QueryResult res = table.query(kuku::make_item(v, 0));
        id_loc_map[res.location()] = v;
    });
    return id_loc_map;
}

std::map<uint64_t, std::vector<uint64_t>> SimpleHash(
    const std::vector<uint64_t> &ids, const PsiAnalyticsContext &context)
{
    std::vector<kuku::LocFunc> funcs;
    for (size_t i = 0; i < context.nfuns; i++) {
        funcs.emplace_back(context.nbins, kuku::make_item(i, 0));
    }
    std::map<uint64_t, std::vector<uint64_t>> id_loc_map;

    std::for_each(ids.begin(), ids.end(), [&id_loc_map, &funcs](const uint64_t &v) {
        std::vector<uint64_t> loc;
        std::for_each(funcs.begin(), funcs.end(), [&v, &loc](const kuku::LocFunc &f) {
            loc.push_back(f(kuku::make_item(v, 0)));
        });
        id_loc_map[v] = loc;
    });

    return id_loc_map;
}

void PrintInfo(const PsiAnalyticsContext &context)
{
    std::cout << "Time for wait " << context.timings.wait << "ms\n";
    std::cout << "Time for hashing " << context.timings.hashing << " ms\n";
    std::cout << "Time for OPRF " << context.timings.oprf1st << " ms\n";
    std::cout << "Time for polynomials " << context.timings.polynomials << " ms\n";
    std::cout << "Time for transmission of the polynomials "
              << context.timings.polynomials_transmission << " ms\n";
    std::cout << "Total runtime: " << context.timings.total << "ms\n";
    std::cout << "Total runtime-wait time: " << context.timings.total - context.timings.wait
              << "ms\n";
    std::cout << "Total receive: " << context.totalReceive * 1.0 / 1048576 << "MB\n";
    std::cout << "Total send: " << context.totalSend * 1.0 / 1048576 << "MB\n";
}