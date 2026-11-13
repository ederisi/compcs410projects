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
#include <filesystem>
#include <fcntl.h>
#include <signal.h>
#include "websocket.h" 

#define MAX 2048
#define PORT 8080 
#define SA struct sockaddr
#define MSG_INTERVAL 3

// Enables background daemon mode by redireccting stdout and stderr
// to a local 'ws_server.log' file next to the executable
void enableLogging() {
    // Resolve the directory of the currently running executable
    std::filesystem::path exe_dir = std::filesystem::canonical("/proc/self/exe").parent_path();
    std::filesystem::path log_path = exe_dir / "ws_server.log";

    int log_fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        std::cerr << "Failed to open log file: " << log_path << "\n";
        return;
    }

    // Redirect stdout and stderr to the log file
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    close(log_fd);

    // Flush output immediately so logs appear in real-time
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

// Driver function 
int main(int argc, char* argv[])
{
    // Ignore SIGPIPE to prevent the server from crashing
    // (if a client disconnect abruptly)
    signal(SIGPIPE, SIG_IGN); 

    // Detect if background logging mode (-b) is requested
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-b") {
            enableLogging();
            break;
        }
    }

    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // Create socket and verification success
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        std::cerr << "WSServer Socket creation failed. Exiting." << "\n\n";
        return 1; 
    } 

    // Allow immediate port reuse if the server is restarted via Ctrl+C
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bzero(&servaddr, sizeof(servaddr)); 
  
    // Assign IP, PORT configs
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        std::cerr << "WSServer Socket bind failed. Exiting." << "\n\n";
        close(sockfd);
        return 1; 
    } 
  
    // Set socket to listening mode to accept incoming client connections
    if ((listen(sockfd, 5)) != 0) { 
        std::cerr << "WSServer failed to listen socket. Exiting." << "\n\n";
        close(sockfd);
        return 1; 
    } 

    // Main server loop: accepts one client, keeps waiting for a
    // new client whenever the current one fails or closes
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

        // Read the client's incoming HTTP request into the buffer
        char buff[MAX] = {0};
        ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1);
        if (bytes_read <= 0) {
            std::cerr << "WSServer failed to read client's HTTP request. Dropping client.\n\n"; 
            close(connfd);
            continue; 
        }   
        std::string request(buff, bytes_read);

        // Try extracting Sec-WebSocket-Key from client's handshake request
        std::string client_key = WEBSOCKET::extractClientKey(request);
        if (client_key.empty()) {
            std::cerr << "WSServer could not extract Sec-WebSocket-Key. Dropping client.\n\n";
            close(connfd);
            continue; 
        }

        // Build and send Websocket handshake response to client 
        std::string http_response = WEBSOCKET::buildHandshakeResponse(client_key);
        ssize_t bytes_sent = send(connfd, http_response.c_str(), http_response.length(), MSG_NOSIGNAL);

        if (bytes_sent <= 0) {
            std::cerr << "WSServer failed to send WS promotion handshake. Dropping client.\n\n";
            close(connfd);
            continue; 
        } 

        // Handshake successfull: Enter loop sending time to the connected client 
        std::cout << "Handshake sent successfully. Starting to send time..." << std::endl;
        bool client_connected = true;

        while (client_connected) {
            // Generate current time payloed wrapped in Websocket text frame
            auto frame = WEBSOCKET::buildTimeFrame();

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
