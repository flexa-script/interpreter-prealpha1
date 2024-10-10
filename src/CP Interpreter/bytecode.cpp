#include "bytecode.hpp"

uint8_t* byteopnd_s(const std::string& str) {
    uint64_t size = static_cast<uint64_t>(str.size());
    size_t total_size = sizeof(size) + str.size();
    uint8_t* concat = new uint8_t[total_size];
    std::memcpy(concat, &size, sizeof(size));
    std::memcpy(concat + sizeof(size), str.data(), str.size());
    return byteopnd(concat);
}
