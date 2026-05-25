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
#include <vector>
#include <thread>

#include "protocols_TCP.h"
#include "UDP_class.h"

bool can_continue = false;
bool resend= false;
bool connected = false;
bool is_tcp = false;

UDPClient client;
std::vector<std::string> all_msgs;


void reader(int in_socket)
{
    int n;

    while(connected)
    {
        char PROTOCOL;
        int n = read(in_socket, &PROTOCOL, 1);

        if (n <= 0)
        {
            connected = false;
            break;
        }

        switch (PROTOCOL)
        {
        case 'b':
        {
            auto data = prt_recv_TCP::broadcast_response(in_socket);
            std::string msg_to_push = data.first + ": " + data.second;
            all_msgs.push_back(msg_to_push);

            break;
        }
        case 'u':
        {
            auto data = prt_recv_TCP::unicast_response(in_socket);
            std::string msg_to_push = data.first + ": " + data.second;
            all_msgs.push_back(msg_to_push);
            break;
        }
        case 't':
        {  
            auto clients_list = prt_recv_TCP::list_response(in_socket);

            std::string msg_to_push;
            msg_to_push += "Server: The clients are:  ";

            for(auto &c : clients_list)
                msg_to_push += c + " ";

            all_msgs.push_back(msg_to_push);

            break;
        }
        case 'f':
        {
            std::string file_name, file, ori;
            prt_recv_TCP::file_response(file_name, file, ori, in_socket);

            all_msgs.push_back(std::string("SERVER: File received"));
            write_binary_file("new_" + file_name , file);

            break;
        }
        case 'E':
        {
            std::string error = prt_recv_TCP::error(in_socket);

            std::string msg_to_push = "ERROR: " + error;
            all_msgs.push_back(msg_to_push);
            break;
        }

        case 'K':
        {
            std::string msg_to_push = "SERVER: K";
            all_msgs.push_back(msg_to_push);
            break;
        }
        default:
        {
            std::cout << "Unrecognized command! (" << PROTOCOL << ")";
            break;
        }
        }
    }

    close(in_socket);
}

void reader_UDP(int in_socket)
{
    struct sockaddr_in sender_addr;
    
    while(connected)
    {
        std::string msg = client.recv_fragment(in_socket, sender_addr);
        
        if (msg.empty())
            continue;

        char opt = msg.front();
        msg = msg.substr(1);

        switch(opt)
        {
        case 'F':
        {
            std::string ori, dest, file_name, file;
            bool ok = client.parse_file(msg, ori, dest, file_name, file);
            
            
            if (!ok)
            {
                client.send_message_udp_AN(in_socket, sender_addr, 'N', 0, 0);
                continue;
            }
            
            
            client.send_message_udp_AN(in_socket, sender_addr, 'K', 0, 0);

            write_binary_file("new_" + file_name, file);
            all_msgs.push_back("SERVER: File received");
            break;
        }
        case 'E':
        {
            std::string error_msg;
            bool ok = client.parse_error(msg, error_msg);

            all_msgs.push_back("ERROR: " + error_msg);
            break;
        }
        case 'b':
        {
            std::string in_nick, in_msg;
            bool ok = client.parse_broadcast(msg, in_nick, in_msg);
            std::string msg_to_push = in_nick + ": " + in_msg;
            all_msgs.push_back(msg_to_push);

            break;
        }
        case 'u':
        {
            std::string in_nick, in_msg;
            bool ok = client.parse_unicast(msg, in_nick, in_msg);

            if (ok)
            {
                std::string msg_to_push = in_nick + ": " + in_msg;
                all_msgs.push_back(msg_to_push);

                client.send_message_udp_AN(in_socket, sender_addr, 'K', 0, 0);
            }
            else
                client.send_message_udp_AN(in_socket, sender_addr, 'N', 0, 0);
            break;
        }
        case 't':
        {  
            std::vector<std::string> clients_list;

            bool ok = client.parse_list(msg, clients_list);

            std::string msg_to_push;
            msg_to_push += "Server: The clients are:  ";

            for(auto &c : clients_list)
                msg_to_push += c + " ";

            all_msgs.push_back(msg_to_push);

            break;
        }
        case 'K':
        {
            can_continue = true;
            break;
        }
        case 'N':
        {
            resend = true;
            can_continue = false;
            break;
        }
        default:
        {
            std::string un_command = "Unrecognized command! (" + std::string(1, opt) + ")";
            all_msgs.push_back(un_command);
            break;
        }
        }
    }
}


int main(void)
{
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD;

    
    int n;

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    
    std::string nick, t_buffer;

    std::cout << "You will use TCP? (1 = Yes, Everything else = NO): ";
    std::getline(std::cin, t_buffer);
    int val = std::stoi(t_buffer);

    is_tcp = (val == 1)? true : false;


    while(true)
    {
        system("clear");


        std::cout << "======= Chat log =======\n";
        std::cout << (is_tcp? "TCP chat" : "UDP chat") << "\n";
        for (auto &msg: all_msgs)
            std::cout << msg << "\n";
        std::cout << "\n\n";
        
        std::cout << "======= MENU CLIENT =======\n";
        if (connected)
            std::cout << "Nick: " << nick << "\n"; 
        std::cout << "Options:\n\n";

        if (!connected)
            std::cout << "1. Login\n";

        if (connected)
        {
            std::cout << "2. Logout\n";
            std::cout << "3. Broadcast\n";
            std::cout << "4. Unicast\n";
            std::cout << "5. List\n";
            std::cout << "6. File\n";
            std::cout << "7. Update chat\n";
        }
            
        std::string buffer;
        std::cout << "Enter option: ";
        std::getline(std::cin, buffer);

        int option = std::stoi(buffer);

        switch (option)
        {
        case 1:
        {
            if (connected)
                break;

            if (is_tcp)
            {
                SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
                connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
            }
            else
                SocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

            do
            {
                std::cout << "Enter nickname: ";
                std::getline(std::cin, nick);
        
                if (is_tcp)
                {
                    prt_send_TCP::login(nick, SocketFD);
                    connected = prt_recv_TCP::k_response(SocketFD);

                    if (connected)
                        std::thread(reader, SocketFD).detach();
                    else
                    {
                        std::string error = prt_recv_TCP::error(SocketFD);
                        std::cout << "\nERROR: " + error << "\n";
                    }
                }
                else
                {
                    client.send_login(nick, SocketFD, stSockAddr);

                    auto a_n_e = client.ack_or_nack(SocketFD, stSockAddr);
                    if (a_n_e == 'K')
                    {
                        std::thread(reader_UDP, SocketFD).detach();
                        connected = true;
                    }
                    else if (a_n_e == 'N')
                    {
                        client.send_login(nick, SocketFD, stSockAddr);
                    }
                    else
                        all_msgs.push_back("Server: Username already taken!");
                    
                }
                
            } while (!connected);
            
            
            break;
        }
        case 2:
        {
            if (connected)
            {
                if (is_tcp)
                    prt_send_TCP::logout(SocketFD);
                else
                    client.send_logout(nick, SocketFD, stSockAddr);
                connected = false;
            }
            else
                std::cout << "Please login first!\n";
            
            break;
        }
        case 3:
        {
            if (connected)
            {
                std::string msg;
                std::cout << "Enter message: ";
                std::getline(std::cin, msg);
                
                if (is_tcp)
                    prt_send_TCP::broadcast(msg, SocketFD);
                else
                {
                    client.send_broadcast(msg, nick, SocketFD, stSockAddr);
                    while(!can_continue)
                    {
                        if (resend)
                        {
                            client.send_broadcast(msg, nick, SocketFD, stSockAddr);
                            resend = false;
                        }
                    }
                    can_continue = false;
                }
            }
            else
            std::cout << "Please login first!\n";

            break;
        }
        case 4:
        {
            if (connected)
            {
                std::string dst;
                std::cout << "Enter destinatary: ";
                std::getline(std::cin, dst);
        
                std::string msg;
                std::cout << "Enter message: ";
                std::getline(std::cin, msg);
        
                if (is_tcp)
                    prt_send_TCP::unicast(msg, dst, SocketFD);
                else
                {
                    client.send_unicast(msg, dst, nick, SocketFD, stSockAddr);
                    while(!can_continue)
                    {
                        if (resend)
                        {
                            client.send_unicast(msg, dst, nick, SocketFD, stSockAddr);
                            resend = false;
                        }
                    }

                    can_continue = false;
                }
            }
            else
                std::cout << "Please login first!\n";
            break;
        }
        case 5:
        {
            if (connected)
            {
                if (is_tcp)
                    prt_send_TCP::list(SocketFD);
                else
                {
                    client.send_list(SocketFD, stSockAddr);
                    can_continue = false;
                }
            }
            else
                std::cout << "Please login first!\n";
            break;
        }
        case 6:
        {
            if (connected)
            {
                std::string file_name;
                std::cout << "Enter file name: ";
                std::getline(std::cin, file_name);
        
                std::string dst;
                std::cout << "Enter destinatary: ";
                std::getline(std::cin, dst);

                std::string file = read_binary_file(file_name);
                if (file.empty())
                {
                    all_msgs.push_back(std::string("ERROR: Empty or unexistent file"));
                    break;
                }

                if (is_tcp)
                    prt_send_TCP::file(file_name, file, dst, SocketFD);
                else
                {
                    client.send_file(file_name, file, dst, nick, SocketFD, stSockAddr);
                    while (!can_continue)
                    {
                        if (resend)
                        {
                            client.send_file(file_name, file, dst, nick, SocketFD, stSockAddr);
                            resend = false;
                        }
                    }

                    can_continue = false;
                }
            }
            else
                std::cout << "Please login first!\n";
            break;
        }
        default:
            break;
        }
    }

    close(SocketFD);
    return 0;
}
