#include "joinData.h"
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <volePSI/Defines.h>
#include "csv.hpp"

joinData::joinData(const PsiAnalyticsContext &context)
{
    auto &configs = context.role == PA ? context.pa_config : context.pb_config;
    volePSI::PRNG prng({ 526, 108 });
    switch (context.role) {
    case PA:
        features = Matrix(context.pa_elems, context.pa_features);
        prng.get(features.data(), context.pa_elems * context.pa_features);
        break;
    case PB:
        features = Matrix(context.pb_elems, context.pb_features);
        prng.get(features.data(), context.pb_elems * context.pb_features);
        break;
    }
    if (!context.is_test) {
        csv::CSVReader reader(context.data_file);
        for (auto &row : reader) {
            size_t i = 0;
            uint64_t id = std::atoi(row[0].get().c_str());
            ids.push_back(id);
            for (size_t j = 1; j < row.size(); j++) {
                block b;
                switch (configs[j - 1].type) {
                case NUM: {
                    to_block(row[j].get<long double>(), b);
                } break;
                case TEXT: {
                    to_block(row[j].get<std::string>(), b);
                    break;
                }
                case BOOL:
                    to_block(row[j].get<bool>(), b);
                    break;
                }
                features(i, j) = b;
            }
            i++;
        }
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
