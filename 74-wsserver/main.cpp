#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <signal.h>
#include "websocket.h" 

#define MAX 2048
#define PORT 8080 
#define SA struct sockaddr
#define MSG_INTERVAL 3

// Driver function 
int main() 
{ 
    signal(SIGPIPE, SIG_IGN);

    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        std::cerr << "WSServer Socket creation failed. Exiting." << "\n\n";
        return 1; 
    } 

    // Allow immediate port reuse if the server is restarted via Ctrl+C
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        std::cerr << "WSServer Socket bind failed. Exiting." << "\n\n";
        close(sockfd);
        return 1; 
    } 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        std::cerr << "WSServer failed to listen socket. Exiting." << "\n\n";
        close(sockfd);
        return 1; 
    } 

    // Main server loop: accepts one client, keeps waiting for a new client whenever the current one fails or closes
    while (true) {
        std::cout << "WSServer waiting for a client connection..." << std::endl; 

        struct sockaddr_in cli;
        socklen_t len = sizeof(cli); 
        int connfd = accept(sockfd, (SA*)&cli, &len);

        if (connfd < 0) {
            std::cerr << "WSServer failed to accept a client. Retrying...\n";
            continue; 
        }
        std::cout << "WSServer accepted a client. Executing WS handshake..." << std::endl; 

        // Read the client's HTTP request into a buffer
        char buff[MAX] = {0};
        ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1);
        if (bytes_read <= 0) {
            std::cerr << "WSServer failed to read client's HTTP request. Dropping client.\n\n"; 
            close(connfd);
            continue; 
        }   
        std::string request(buff, bytes_read);

        // Try extracting client key from request
        std::string client_key = WEBSOCKET::extractClientKey(request);
        if (client_key.empty()) {
            std::cerr << "WSServer could not extract Sec-WebSocket-Key. Dropping client.\n\n";
            close(connfd);
            continue; 
        }

        // Build and send handshake response to client 
        std::string http_response = WEBSOCKET::buildHandshakeResponse(client_key);
        ssize_t bytes_sent = send(connfd, http_response.c_str(), http_response.length(), MSG_NOSIGNAL);

        if (bytes_sent <= 0) {
            std::cerr << "WSServer failed to send WS promotion handshake. Dropping client.\n\n";
            close(connfd);
            continue; 
        } 

        // Enter loop sending time to the connected client 
        std::cout << "Handshake sent successfully. Starting to send time..." << std::endl;
        bool client_connected = true;

        while (client_connected) {
            auto frame = WEBSOCKET::buildTimeFrame(); // Get time as TEXT packet frame

            // Send frame
            size_t total_sent = 0;
            while (total_sent < frame.size()) {
                ssize_t sent = send(connfd, 
                                    reinterpret_cast<const char*>(frame.data() + total_sent), 
                                    frame.size() - total_sent, 
                                    MSG_NOSIGNAL);
                if (sent <= 0) {
                    std::cerr << "Client disconnected or socket error occurred.\n\n";
                    client_connected = false;
                    break; // Break out of the inner frame-sending loop
                }
                total_sent += sent;
            }
            
            if (!client_connected) {
                break; // Break out of the client loop to accept a new connection
            }

            // Sleep 3s before next message
            std::this_thread::sleep_for(std::chrono::seconds(MSG_INTERVAL));
        }
        close(connfd); // Close connection 
    }
    close(sockfd); // Close socket
    return 0; 
}
