#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace WEBSOCKET {
    std::string extractClientKey(const std::string& request);
    std::string buildHandshakeResponse(const std::string& clientKey);
    std::vector<unsigned char> buildTimeFrame();

    static std::string base64Encode(const unsigned char* bytes, size_t len);
    static std::string generateAcceptKey(const std::string& clientKey);
}

#endif