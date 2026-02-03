#include <sys/socket.h>    // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>    // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>     // htons(), htonl(), inet_addr()
#include <unistd.h>        // close(), read(), write()
#include <cstring>         // memset()
#include <iostream>

/*
// For connect, you need the same address structure:
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Connect to localhost

int result = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
*/


#define PORT 8080

int main()
{
    // 1. Create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }
    
    // 2. Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    // TODO: Fill in server_addr
    //   - sin_family
    //   - sin_port (8080)
    //   - sin_addr (127.0.0.1)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // 3. Connect to server
    int result = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (result < 0)
    {
        std::cerr << "Connection failed\n";
        return 1;
    }
    
    // 4. Send message
    // TODO: Call send() with "Hello from client!"
    const char* message = "Hello from client";
    send(client_fd, message, strlen(message), 0);
    
    // 5. Receive response
    // TODO: Call recv() and print result
    char buffer[1024] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';  // Null terminate
        std::cout << "Received: \n" << buffer << std::endl;
    }
    
    // 6. Cleanup
    close(client_fd);

    return 0;
}
