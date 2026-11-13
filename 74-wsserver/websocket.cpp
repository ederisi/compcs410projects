#include "websocket.h"

// Extracts the 'Sec-WebSocket-Key' header value from an incoming HTTP req
std::string WEBSOCKET::extractClientKey(const std::string& request) {
    std::string client_key;
    std::string search_str = "Sec-WebSocket-Key: ";

    // Search request for the key
    size_t key_pos = request.find(search_str);
    
    if (key_pos != std::string::npos) {
        // Move the position forward to the start of the actual key string
        size_t start_pos = key_pos + search_str.length();

        // Find the end of the line (HTTP headers always end with \r\n)
        size_t end_pos = request.find("\r\n", start_pos);

        if (end_pos != std::string::npos) {
            // Extract key substring
            client_key = request.substr(start_pos, end_pos - start_pos);
        }
    }
    return client_key; 
}

// Builds the HTTP 101 Switching protocol response
std::string WEBSOCKET::buildHandshakeResponse(const std::string& clientKey) {
    // Encode accept key to return to client with SHA-1 
    std::string accept_key = generateAcceptKey(clientKey); 

    // Build response payload
    std::string http_response = 
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";

    return http_response; 
}

// Generates a WebSocket text frame containing current system time
std::vector<unsigned char> WEBSOCKET::buildTimeFrame() {
    // Get current time and convert to text
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string time_str = std::ctime(&time);
    size_t length = time_str.size();


    std::vector<unsigned char> frame;
    // Push WebSocket header byte: FIN-bit (0x80) + TEXT-opcode (0x01)
    frame.push_back(0x81);
    
    // Add time length
    frame.push_back(static_cast<unsigned char>(length));

    // add actual time information to frame
    for (char c : time_str) {
        frame.push_back(c);
    }

    return std::move(frame);
}

// Individual Base64-coder
std::string WEBSOCKET::base64Encode(const unsigned char* bytes, size_t len) {
    const std::string b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];

    // process input data 3 bytes at a time
    while (len--) {
        char_array_3[i++] = *(bytes++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += b64chars[char_array_4[i]];
            i = 0;
        }
    }

    // Handle remaining trailing bytes and apply padding
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) ret += b64chars[char_array_4[j]];
        while (i++ < 3) ret += '=';
    }
    return ret;
}


// Minimalistic and individual SHA-1
#define SHA1_ROTL(bits, word) (((word) << (bits)) | ((word) >> (32-(bits))))
std::string WEBSOCKET::generateAcceptKey(const std::string& clientKey) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string text = clientKey + magic;
    
    // Initialize SHA-1 constants
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    
    // Text padding
    uint64_t ml = text.length() * 8;
    std::vector<uint8_t> msg(text.begin(), text.end());
    msg.push_back(0x80);
    while ((msg.size() * 8) % 512 != 448) msg.push_back(0);
    for (int i = 7; i >= 0; --i) msg.push_back((ml >> (i * 8)) & 0xFF);

    // Process padded message in 512 bit (64 byte) blocks
    for (size_t offset = 0; offset < msg.size(); offset += 64) {
        uint32_t w[80];
        // Break block into sixteen 32-bit words
        for (int i = 0; i < 16; ++i) {
            w[i] = (msg[offset + i*4] << 24) | (msg[offset + i*4 + 1] << 16) | 
                   (msg[offset + i*4 + 2] << 8) | (msg[offset + i*4 + 3]);
        }
        // Extend sixteen 32-bit words into eighty 32-bit words
        for (int i = 16; i < 80; ++i) {
            w[i] = SHA1_ROTL(1, w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

        // Main SHA-1 mixing loop
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            uint32_t temp = SHA1_ROTL(5, a) + f + e + k + w[i];
            e = d; d = c; c = SHA1_ROTL(30, b); b = a; a = temp;
        }
        // Accumulate intermediate block hash results into global state
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    // Shift to byte array
    unsigned char hash[20];
    uint32_t h[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        hash[i*4] = (h[i] >> 24) & 0xFF;
        hash[i*4+1] = (h[i] >> 16) & 0xFF;
        hash[i*4+2] = (h[i] >> 8) & 0xFF;
        hash[i*4+3] = h[i] & 0xFF;
    }

    return base64Encode(hash, 20);
}