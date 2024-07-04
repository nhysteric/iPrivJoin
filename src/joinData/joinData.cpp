#include "joinData.h"
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "csv.hpp"
#include "toml.hpp"
struct config {
    std::string name;
    serializeType type;
};

void joinData::print()
{
    for (auto &ff : features) {
        for (auto &feature : ff) {
            switch (feature.type) {
            case BOOL: {
                bool b = from_block<bool>(feature.data);
                std::cout << b<<"\n";
            } break;
            case NUM: {
                long double num = from_block<long double>(feature.data);
                std::cout << num<<"\n";
            } break;
            case TEXT: {
                std::string s = from_block<std::string>(feature.data);
                std::cout << s<<"\n";
            } break;
            }
        }
    }
}

joinData::joinData(const std::string &config_file, const std::string &data_file)
{
    std::vector<config> configs;
    auto table = toml::parse_file(config_file);
    if (table["feature"].is_array_of_tables()) {
        for (auto &feature : *table["feature"].as_array()) {
            auto f = *feature.as_table();
            config c;
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
            configs.push_back(c);
        }
    }

    csv::CSVReader reader(data_file);
    for (auto &row : reader) {
        uint64_t id = std::atoi(row[0].get().c_str());
        ids.push_back(id);
        std::vector<feature> feature_row;
        for (size_t index = 1; index < row.size(); index++) {
            feature f(configs[index].name);
            switch (configs[index - 1].type) {
            case NUM: {
                to_block(row[index].get<long double>(), f.data);
                f.type = NUM;
            } break;
            case TEXT: {
                to_block(row[index].get<std::string>(), f.data);
                f.type = TEXT;
                break;
            }
            case BOOL:
                to_block(row[index].get<bool>(), f.data);
                f.type = BOOL;
                break;
            }
            feature_row.push_back(f);
        }
        features.push_back(feature_row);
    }
}

// joinData::joinData(std::string config_file, std::string data_file)
// {

//     csv::CSVReader reader(data_file);
//     for (auto &row : reader) {
//         uint64_t id = std::atoi(row[0].get().c_str());
//         ids.push_back(id);
//         std::vector<feature> feature_row;
//         for (size_t index = 1; index < row.size(); index++) {
//             feature f(configs[index - 1].name, configs[index - 1].size);
//             switch (configs[index - 1].type) {
//             case INT:
//                 serialize(row[index].get<block>(), f.data, configs[index - 1].size);
//                 f.type = INT;
//                 break;
//             case FlOAT:
//                 serialize(row[index].get<float>(), f.data, configs[index - 1].size);
//                 break;
//             case TEXT:
//                 serialize(row[index].get<std::string>(), f.data, configs[index - 1].size);
//                 f.type = TEXT;
//                 break;
//             case BOOL:
//                 serialize(row[index].get<bool>(), f.data, configs[index - 1].size);
//                 f.type = BOOL;
//                 break;
//             }
//             feature_row.push_back(f);
//         }
//         features.push_back(feature_row);
//     }
// }