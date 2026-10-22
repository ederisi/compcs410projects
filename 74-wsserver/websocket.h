#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <string>
#include <vector>
#include <cstdint>

std::string base64_encode(const unsigned char* bytes, size_t len);
std::string generateAcceptKey(const std::string& clientKey);

#endif