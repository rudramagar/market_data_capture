#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdint>
#include <string>
#include <vector>

struct ChannelConfig {
    std::string multicast_address;
    uint16_t    multicast_port = 0;
    std::string interface;
    std::string source;
};

struct RecoveryChannelConfig {
    std::string rerequester_address;
    uint16_t    rerequester_port            = 0;
    uint16_t    max_recovery_message_count  = 5000;
};

struct ServiceConfig {
    uint64_t next_sequence_number   = 0;
    int      queue_size             = 8 * 1024 * 1024;
    std::vector<ChannelConfig> channels;
    std::vector<RecoveryChannelConfig> recovery_channels;
};

bool load_config(const char* yaml_path);
const ServiceConfig& config();

#endif
