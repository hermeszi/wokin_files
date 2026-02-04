#include <sys/socket.h>    // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>    // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>     // htons(), htonl(), inet_addr()
#include <unistd.h>        // close(), read(), write()
#include <cstring>         // memset()
#include <iostream>
#include <vector>

/*
struct sockaddr_in
{
    sa_family_t    sin_family;  // Address family (AF_INET = IPv4)
    in_port_t      sin_port;    // Port number
    struct in_addr sin_addr;    // IP address
};

- `sin_family = AF_INET` → "I'm using IPv4"
- `sin_port = htons(8080)` → "Bind to port 8080"
- `sin_addr.s_addr = INADDR_ANY` → "Listen on all network interfaces"

like an address label:

sin_family:  "Use IPv4 addressing"
sin_addr:    "Any IP on this machine"
sin_port:    "Port 8080"
*/

#define PORT 8080

int main()
{
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }
    
    
    // 2. Setup address structure
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    address.sin_port = htons(PORT);        // Port 8080
    
    // Allow port reuse, without waiting
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "set port for reuse failed\n";
    }

    // 3. Bind socket to port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed\n";
        return 1;
    }

    /*including poll*/
    std::vector<pollfd> fds;
    pollfd server_pollfd = {server_fd, POLLIN, 0};
    fds.push_back(server_pollfd);

    while (true) {
        poll(&fds[0], fds.size(), -1);
        
        // Check each fd in the array
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd)
                {
                    // 5. Accept a connection
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd < 0)
                    {
                        std::cerr << "Accept failed\n";
                        close(server_fd);
                        return 1;
                    }
                    char* client_ip = inet_ntoa(client_addr.sin_addr);  // Convert IP to string
                    int client_port = ntohs(client_addr.sin_port);      // Convert port to host order
                
                    std::cout << "Client " << client_ip << ":" << client_port << " connected on fd " << client_fd << std::endl;
                } 
                else 
                {
                    // TODO: Handle client data
                }
            }
        }
    }
    
    // 4. Listen for connections
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    
    std::cout << "Server listening on port "<< PORT << "...\n";
    

    // 6. Receive data
    char buffer[1024] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';  // Null terminate
        std::cout << "Received: \n" << buffer << std::endl;
        
        // 7. Send response
        const char* response = "Message received!";
        send(client_fd, response, strlen(response), 0);
    }
    
    // 8. Cleanup
    close(client_fd);
    close(server_fd);
    std::cout << "Server closed\n";
    
    return 0;
}
