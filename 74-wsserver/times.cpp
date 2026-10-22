#include "times.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <sys/socket.h>

bool sendTime(int client_fd) {
    while (true) {
        // Find current time and convert to text
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string time_str = std::ctime(&time);
        size_t length = time_str.size();


        std::vector<unsigned char> frame;
        frame.push_back(0x81); // FIN-bit (0x80) and TEXT-opcode (0x01)
        
        // Add time length
        frame.push_back(static_cast<unsigned char>(length));

        // add actual time information to frame
        for (char c : time_str) {
            frame.push_back(c);
        }

        size_t total_sent = 0;
        while (total_sent < frame.size()) {
                ssize_t sent = send(client_fd, 
                            reinterpret_cast<const char*>(frame.data() + total_sent), 
                            frame.size() - total_sent, 
                            0);
                if (sent <= 0) {
                    return false; // Connection closed or socket error
                    }
                total_sent += sent;
            }
        
        // put thread to sleep
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    return true;

}