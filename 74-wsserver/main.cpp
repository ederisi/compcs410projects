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
#define MAX 2048
#define PORT 8080 
#define SA struct sockaddr
#include "websocket.h" 
#include "times.h"
  
#include <cstdint>
#include <vector>

// Driver function 
int main() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 

    connfd = accept(sockfd, (SA*)&cli, (socklen_t*)&len);

    if (connfd < 0) {
        printf("server accept failed.. \n");
        exit(0);
    }

    else {
        printf("server accepted the client... \n");
    }

    char buff[MAX] = {0};
    //  Read the client's HTTP request into the buffer
    ssize_t bytes_read = read(connfd, buff, sizeof(buff) - 1);
    if (bytes_read <= 0) {
        close(connfd);
        exit(0);
    }   

    // Convert it directly to a string
    std::string request(buff, bytes_read);

    std::string client_key;
    std::string search_str = "Sec-WebSocket-Key: ";

    //  Now you can easily search the string for the key
    size_t key_pos = request.find(search_str);
    
    if (key_pos != std::string::npos) {
        // Move the position forward to the start of the actual key string
        size_t start_pos = key_pos + search_str.length();

        // Find the end of the line (HTTP headers always end with \r\n)
        size_t end_pos = request.find("\r\n", start_pos);

        if (end_pos != std::string::npos) {
            // Extract just the key by slicing the string (start index, length)
            client_key = request.substr(start_pos, end_pos - start_pos);
        
            printf("Successfully extracted key: %s\n", client_key.c_str());
        }

    }
    std::string acceptkey = generateAcceptKey(client_key);

    // Build HTTP response
    std::string http_response = 
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: " + acceptkey + "\r\n\r\n";

    // Send response to client 
    ssize_t bytes_sent = write(connfd, http_response.c_str(), http_response.length());

    if (bytes_sent > 0) {
        printf("Handshake sent successfully. Starting to send time...\n");
    
        // Handshake ready
        sendTime(connfd);
    } else {
        printf("Failed to send handshake.\n");
    }

}
