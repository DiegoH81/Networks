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

#include "protocols_TCP.h"
#include "UDP_class.h"
#include "aux_funcs.h"

std::map<std::string, struct sockaddr_in> clients;
std::map<std::string, int> clientsTCP;
std::mutex clients_mutex;
UDPServer server;

bool is_tcp = false;


void reader(int in_socket)
{
    int n;
    std::string nick;
    // READ

    bool listening = true;
    while(listening)
    {
        char protocol;
        int n = read(in_socket, &protocol, 1);

        if (n <= 0)
        {
            listening = false;

            std::unique_lock<std::mutex> lock(clients_mutex);
            if (clientsTCP.find(nick) != clientsTCP.end())
                clientsTCP.erase(nick);
            lock.unlock();

            break;
        }

        switch (protocol)
        {
        case 'L':
        {
            nick = prt_recv_TCP::login(in_socket);

            std::unique_lock<std::mutex> lock(clients_mutex);
            if (clientsTCP.find(nick) != clientsTCP.end())
            {
                lock.unlock();
                prt_send_TCP::error("Username already taken!", in_socket);
                break;
            }

            clientsTCP[nick] = in_socket;
            lock.unlock();

            prt_send_TCP::k_response(in_socket);
            
            std::unique_lock<std::mutex> lock_B(clients_mutex);
            
            std::cout << "\n";
            for (auto& i: clientsTCP)
                std::cout << "C: " << i.first << " - Socket: " << i.second <<"\n";
            lock_B.unlock();

            break;
        }
        case 'B':
        {
            auto data = prt_recv_TCP::broadcast(in_socket);
            
            std::unique_lock<std::mutex> lock(clients_mutex);

            for (auto &c : clientsTCP)
                prt_send_TCP::broadcast_response(data, nick, c.second);
            lock.unlock();

            break;
        }
        case 'U':
        {
            auto data = prt_recv_TCP::unicast(in_socket);
            auto &msg = data.first;
            auto &in_nick = data.second;

            std::unique_lock<std::mutex> lock(clients_mutex);

            if (clientsTCP.find(in_nick) == clientsTCP.end())
            {
                prt_send_TCP::error("User not found!", in_socket);
                lock.unlock();
                break;
            }

            prt_send_TCP::unicast_response(msg, nick, clientsTCP[in_nick]);
            lock.unlock();
            break;
        }
        case 'T':
        {
            std::vector<std::string> clients_vec;

            std::unique_lock<std::mutex> lock(clients_mutex);

            for (auto &c : clientsTCP)
                clients_vec.push_back(c.first);

            prt_send_TCP::list_response(clients_vec, clientsTCP[nick]);
            lock.unlock();

            break;
        }
        case 'F':
        {
            std::string file, file_name, dest;

            prt_recv_TCP::file_response(file_name, file, dest, in_socket);
            
            std::unique_lock<std::mutex> lock(clients_mutex);

            if (clientsTCP.find(dest) == clientsTCP.end())
            {
                prt_send_TCP::error("User not found!", in_socket);
                lock.unlock();
                break;
            }

            prt_send_TCP::file_response(file_name, file, nick, clientsTCP[dest]);
            lock.unlock();

            break;
        }
        case 'O':
        {
            prt_send_TCP::k_response(in_socket);
            std::unique_lock<std::mutex> lock(clients_mutex);
            clientsTCP.erase(nick);

            std::cout << "\n";
            for (auto& i: clientsTCP)
                std::cout << "C: " << i.first << " - Socket: " << i.second <<"\n";
            lock.unlock();
            
            listening = false;
            break;
        }
            default:
            break;
        }
    
    }

    close(in_socket);
}

void udp_reader(int in_socket)
{
    struct sockaddr_in sender_addr;
    
    while(true)
    {
        std::string msg = server.recv_fragment(in_socket, sender_addr);
        
        if (msg.empty())
            continue;
        

        char opt = msg.front();
        msg = msg.substr(1);
        
        switch(opt)
        {
        case 'L':
        {
            std::string in_nick;
            bool arrived_ok = server.parse_login(msg, in_nick);

            if (arrived_ok)
            {
                if (clients.find(in_nick) != clients.end())
                {
                    server.send_error("Username already taken!", in_socket, sender_addr);
                    break;
                }

                clients[in_nick] = sender_addr;

                std::cout << "\n\n";
                for (auto &c : clients)
                    std::cout << c.first << "\n";

                server.send_ack(in_socket, sender_addr);
            }
            else
                std::cout << "Couldn't login\n";

            break;
        }
        case 'B':
        {
            std::string in_msg, in_nick;
            bool receieved_msg = server.parse_broadcast(msg, in_msg, in_nick);
            
            if (!receieved_msg)
            {
                std::cout << "Corrupted data\n";
                continue;
            }

            for (auto &c : clients)
                server.send_broadcast(in_msg, in_nick, in_socket, c.second);
            
            break;
        }
        case 'F':
        {
            std::string ori, dest, file_name, file;
            bool Receieved_file = server.parse_file(msg, ori, dest, file_name, file);
            
            if (!Receieved_file)
                continue;
            
            if (clients.find(dest) == clients.end())
            {
                server.send_error("Client not found!", in_socket, sender_addr);
                break;
            }

            server.send_file(file_name, file, dest, ori, in_socket, clients[dest]);
            break;
        }
        case 'U':
        {
            std::string in_nick, in_msg, in_ori;
            bool receieved_unicast = server.parse_unicast(msg, in_nick, in_msg, in_ori);

            if (clients.find(in_nick) == clients.end())
            {
                server.send_error("Client not found!", in_socket, sender_addr);
                break;
            }
            
            server.send_unicast(in_msg, in_ori, in_socket, clients[in_nick]);
            
            break;
        }
        case 'T':
        {
            std::vector<std::string> clients_vec;

            for (auto &c : clients)
                clients_vec.push_back(c.first);

            server.send_list(clients_vec, in_socket, sender_addr);

            break;
        }
        case 'O':
        {
            std::string in_nick;

            bool receieved_logout = server.parse_logout(msg, in_nick);

            clients.erase(in_nick);

            break;
        }
        default:
        break;
        }        
    }
}


int main(void)
{
    struct sockaddr_in stSockAddr;

    std::string nick, buffer;

    std::cout << "You will use TCP? (1 = Yes, Everything else = NO): ";
    std::getline(std::cin, buffer);
    int val = std::stoi(buffer);

    is_tcp = (val == 1);

    int SocketFD;

    if (is_tcp)
        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        SocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);


    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

    if (is_tcp)
        listen(SocketFD, 10);
    
    printf("========= SERVER TERMINAL =========\n");
    
    if (!is_tcp)
        std::thread(udp_reader, SocketFD).detach();

    while (true)
    {
        if (is_tcp)
        {
            int ConnectFD = accept(SocketFD, NULL, NULL);
            std::thread(reader, ConnectFD).detach();
        }
    }
    
    close(SocketFD);
    return 0;
}


