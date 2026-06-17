#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <string>
#include <iostream>
#include <thread>
#include <map>
#include <thread>
#include <mutex>

//#include "protocols_TCP.h"
#include "UDP_class.h"
#include "aux_funcs.h"

std::map<std::string, struct sockaddr_in> clients;
UDPServer server;

bool can_continue = false;


int main(void)
{
    struct sockaddr_in stSockAddr;

    std::string nick, buffer;
    int SocketFD;

    
    SocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

    //system("clear");
    printf("========= SERVER TERMINAL =========\n");
    
    
    struct sockaddr_in sender_addr;
    
    while(true)
    {
        std::string msg = server.recv_fragment(SocketFD, sender_addr);
        
        if (msg.empty())
            continue;
        
        char opt = msg.front();
        msg = msg.substr(1);
        
        switch(opt)
        {
        case 'L':
        {
            std::string in_nick;
            server.parse_login(msg, in_nick);

            if (clients.find(in_nick) != clients.end())
            {
                server.send_error("Username already taken!", SocketFD, sender_addr);
                break;
            }

            server.send_ack(SocketFD, sender_addr);
            clients[in_nick] = sender_addr;

            std::cout << "\n\n";
            for (auto &c : clients)
                std::cout << c.first << "\n";

            break;
        }
        case 'F':
        {
            auto t_start = std::chrono::high_resolution_clock::now();

            std::string ori, dest, file_name, file;
            server.parse_file(msg, ori, dest, file_name, file);
            
            if (clients.find(dest) == clients.end())
            {
                server.send_error("Client not found!", SocketFD, sender_addr);
                break;
            }

            
            server.send_file(file_name, file, dest, ori, SocketFD, clients[dest]);
            server.send_ack(999, sender_addr);

             auto t_end = std::chrono::high_resolution_clock::now();
            double processing_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            std::cout << "\n\n\n\n\n\n\n\n";
            std::cout << "-----------------------------------------------------\n"
            std::cout << "NODAL - Processing time: " << processing_ms << " ms\n";
            std::cout << "-----------------------------------------------------\n"
            std::cout << "\n\n\n\n\n\n\n\n";
            break;
        }
        case 'O':
        {
            std::string in_nick;

            server.parse_logout(msg, in_nick);
            clients.erase(in_nick);

            break;
        }
        case 'P':
        {
            server.send_pong(SocketFD, sender_addr);
            break;
        }
        default:
        break;
        }        
    }
    
    close(SocketFD);
    return 0;
}


