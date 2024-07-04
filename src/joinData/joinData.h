#pragma once
#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
enum serializeType { NUM, TEXT, BOOL };

using block = osuCrypto::block;

template <typename T>
void to_block(const T &value, block &b)
{
    std::memset(&b, 0, sizeof(block));
    std::memcpy(&b, &value, (sizeof(T) > 16) ? 16 : sizeof(T));
}

inline void to_block(const std::string &value, block &b)
{
    std::memset(&b, 0, sizeof(block)); // Zero-initialize the block

    std::memcpy(&b, value.data(), (value.size() > 16) ? 16 : value.size());
}

template <typename T>
T from_block(const block &b)
{
    T value;
    std::memset(&value, 0, sizeof(T));
    std::memcpy(&value, &b, (sizeof(T) > 16) ? 16 : sizeof(T));
    return value;
}
template <>
inline std::string from_block(const block &b)
{
    std::string value(16, '\0');    // Create a string of 16 null characters
    std::memcpy(&value[0], &b, 16); // Copy the data from the block

    // Resize the string to remove null characters at the end
    value.resize(std::strlen(value.c_str()));

    return value;
}

class joinData;

struct feature {
private:
    std::string name;
    block data;
    serializeType type;

public:
    feature(const std::string name) : name(name), data(0)
    {}
    feature() = default;
    ~feature() = default;
    feature(const feature &) = default;
    feature(feature &&) = default;
    feature &operator=(const feature &) = default;
    feature &operator=(feature &&) = default;
    friend class joinData;
};

class joinData {
public:
    joinData(const std::string &config_file, const std::string &data_file);
    joinData(const joinData &) = default;
    joinData(joinData &&) = default;
    ~joinData() = default;

    joinData &operator=(const joinData &) = default;
    joinData &operator=(joinData &&) = default;

    const std::vector<std::vector<feature>> &getFeatures()
    {
        return features;
    }

    void print();

private:
    std::vector<uint64_t> ids;
    std::vector<std::vector<feature>> features;
};