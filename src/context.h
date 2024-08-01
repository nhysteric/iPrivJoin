#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "toml.hpp"
using milliseconds_ratio = std::ratio<1, 1000>;
using duration_millis = std::chrono::duration<double, milliseconds_ratio>;
enum SerializeType { NUM, TEXT, BOOL };

enum Role { PA, PB };

struct config {
    std::string name;
    SerializeType type;
};

inline void read_config(std::vector<config> &config, const toml::array &table)
{
    for (auto &feature : table) {
        auto f = *feature.as_table();
        struct config c;
        c.name = f["name"].value_or("1");
        switch (f["type"].value_or(0)) {
        default:
        case 0:
            c.type = NUM;
            break;
        case 1:
            c.type = TEXT;
            break;
        case 2:
            c.type = BOOL;
            break;
        }
        config.push_back(c);
    }
}

struct PsiAnalyticsContext {
    Role role;
    std::string address;
    uint64_t bins;
    uint64_t funcs;
    uint64_t max_in_bin;

    uint64_t pa_elems;
    uint64_t pa_features;
    std::vector<config> pa_config;

    uint64_t pb_elems;
    uint64_t pb_features;
    std::vector<config> pb_config;

    std::string data_file;
    std::string output_file;

    uint64_t layers;

    bool is_test;

    struct {
        double init;
        double hashing;
        double opprf1st;
        double opprf2nd;
        double oprf;
        double total;
        double shuffle1st;
        double shuffle2nd;
        double wait = 0;
    } timings;
    unsigned long totalSend = 0;
    unsigned long totalReceive = 0;

    PsiAnalyticsContext(
        Role role, std::string config_file, std::string data_file, std::string output_file = "")
        : role(role), data_file(data_file), output_file(output_file)
    {
        auto table = toml::parse_file(config_file);
        bins = table["bins"].value_or(0);
        funcs = table["funcs"].value_or(2);
        address = table["address"].value_or("localhost:10011");

        max_in_bin = table["max_in_bin"].value_or(0);

        pa_elems = table["pa_elems"].value_or(0);
        pb_elems = table["pb_elems"].value_or(0);

        pa_features = table["pa_features"].value_or(0);
        pb_features = table["pb_features"].value_or(0);

        layers = table["layers"].value_or(4);
        is_test = table["test"].value_or(true);

        if (!is_test) {
            if (table["pa_feature"].is_array_of_tables()) {
                read_config(pa_config, *table["pa_feature"].as_array());
            }
            if (table["pb_feature"].is_array_of_tables()) {
                read_config(pb_config, *table["pb_feature"].as_array());
            }
        }
    }

    void print()
    {
        std::cout << "Time for init" << timings.init << "ms\n";
        std::cout << "Time for wait " << timings.wait << "ms\n";
        std::cout << "Time for hashing " << timings.hashing << " ms\n";
        std::cout << "Time for OPPRF1st " << timings.opprf1st << " ms\n";
        std::cout << "Time for OPPRF2nd " << timings.opprf2nd << " ms\n";
        std::cout << "Time for OPRF " << timings.oprf << " ms\n";
        std::cout << "Time for shuffle1st " << timings.shuffle1st << " ms\n";
        std::cout << "Time for shuffle2nd " << timings.shuffle2nd << " ms\n";
        std::cout << "Total runtime: " << timings.total << "ms\n";
        std::cout << "Total runtime-wait time: " << timings.total - timings.wait << "ms\n";
        std::cout << "Total receive: " << totalReceive * 1.0 / 1048576 << "MB\n";
        std::cout << "Total send: " << totalSend * 1.0 / 1048576 << "MB\n";

        std::fstream fs(output_file, std::ios::out | std::ios::app);
        fs << "Time for init" << timings.init << "ms\n";
        fs << "Time for wait " << timings.wait << "ms\n";
        fs << "Time for hashing " << timings.hashing << " ms\n";
        fs << "Time for OPPRF1st " << timings.opprf1st << " ms\n";
        fs << "Time for OPPRF2nd " << timings.opprf2nd << " ms\n";
        fs << "Time for OPRF " << timings.oprf << " ms\n";
        fs << "Time for shuffle1st " << timings.shuffle1st << " ms\n";
        fs << "Time for shuffle2nd " << timings.shuffle2nd << " ms\n";
        fs << "Total runtime: " << timings.total << "ms\n";
        fs << "Total runtime-wait time: " << timings.total - timings.wait << "ms\n";
        fs << "Total receive: " << totalReceive * 1.0 / 1048576 << "MB\n";
        fs << "Total send: " << totalSend * 1.0 / 1048576 << "MB\n";
        fs.close();
    }
};
