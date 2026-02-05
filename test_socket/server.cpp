#include <sys/socket.h>    // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>    // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>     // htons(), htonl(), inet_addr()
#include <unistd.h>        // close(), read(), write()
#include <cstring>         // memset()
#include <iostream>
#include <sstream>
#include <vector>
#include <poll.h>     // poll(), pollfd, POLLIN
#include <signal.h>
#include<errno.h>

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

bool g_running = true;

void signal_handler(int signum)
{
    std::cout << signum << "Shutting down server..." << std::endl;
    g_running = false;
}

static int accept_new_connection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
                    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0)
    {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return -1;
    }
    /*these are not saved, but they can be*/
    char* client_ip = inet_ntoa(client_addr.sin_addr);  // Convert IP to string
    int client_port = ntohs(client_addr.sin_port);      // Convert port to host order
                
    std::cout << "Client " << client_ip << ":" << client_port << " connected on fd " << client_fd << std::endl;

    return (client_fd);
}

static void display_chat(char *buffer, int client_fd, int server_fd, std::vector<pollfd> &fds)
{
    std::stringstream ss; 
    ss << "[fd" << client_fd << "]: " << buffer;
    std::string message = ss.str();
    std::cout << message << std::endl;

    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd != client_fd && fds[i].fd != server_fd )
        {
            send(fds[i].fd, message.c_str(), message.size(), 0);
        }
    }
}

int main()
{
    signal(SIGINT, signal_handler);

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

    // 4. Listen for connections
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    
    std::cout << "Server listening on port "<< PORT << "...\n";

    /*including poll for non-block*/
    std::vector<pollfd> fds;

    pollfd stdin_pollfd = {0, POLLIN, 0};  // fd 0 = stdin
    fds.push_back(stdin_pollfd);

    pollfd server_pollfd = {server_fd, POLLIN, 0};
    fds.push_back(server_pollfd);
    std::cout << "Server set-up for multiple clients...\n";

    while (g_running)
    {
        int poll_count = poll(&fds[0], fds.size(), -1);
        if (poll_count < 0)
        {
            if (errno == EINTR)
            {
                // Signal interrupted poll - this is OK, just check running flag
                continue;
            }
            std::cerr << "Poll error\n";
            break;
        }
        
        // Check each fd in the array
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == 0)
                {
                    char server_input[1024] = {0};
                    ssize_t bytes_read = read(fds[i].fd, server_input, sizeof(server_input) - 1);
                    if (bytes_read > 0)
                    {
                        server_input[bytes_read] = '\0';  // Null terminate
                        display_chat(server_input, fds[i].fd, server_fd, fds);
                        std::string server_command(server_input);
                        if (server_command == "quit\n")
                        {
                            std::cout << "QUITING SERVER" << std::endl;
                            g_running = false;
                        }
                    }
                }
                else if (fds[i].fd == server_fd)
                {

                    // 5. Accept a connection
                    int new_client_fd = accept_new_connection(server_fd);
                    if (new_client_fd < 0)
                    {
                        return 1;
                    }
                    else
                    {
                        pollfd client_pollfd = {new_client_fd, POLLIN, 0};
                        fds.push_back(client_pollfd);
                    }
                } 
                else 
                {
                    // 6. Receive data
                    char buffer[1024] = {0};
                    ssize_t bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_read > 0)
                    {
                        buffer[bytes_read] = '\0';  // Null terminate
                        display_chat(buffer, fds[i].fd, server_fd, fds);
                    }
                    else
                    {
                        std::cout << fds[i].fd << " disconnected." << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                }
            }
        }
    }

    // 8. Cleanup
    char shutdown_msg[] = "Shutting down server...";
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd != server_fd)
        {
            std::cout << "Sending shutdown to fd " << fds[i].fd << std::endl;
            send(fds[i].fd, shutdown_msg, strlen(shutdown_msg), 0);
            close(fds[i].fd);
        }
    }
    close(server_fd);
    std::cout << "Server closed\n";
    
    return 0;
}
