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


#include "UDP_class.h"


std::string all_info;

bool connected = false;

UDPClient client;
std::vector<std::string> all_msgs;
std::chrono::high_resolution_clock::time_point file_rtt_start;


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
            client.parse_file(msg, ori, dest, file_name, file);

            write_binary_file("new_" + file_name, file);
            all_msgs.push_back("SERVER: File received");
            break;
        }
        case 'E':
        {
            std::string error_msg;
            client.parse_error(msg, error_msg);

            all_msgs.push_back("ERROR: " + error_msg);
            break;
        }
        case 'G':
        {
            auto t_end = std::chrono::high_resolution_clock::now();
            double ping_ms = std::chrono::duration<double, std::milli>(t_end - client.ping_start).count();
            

            std::cout << "-----------------------------------------------------\n"
            std::cout << "PING: " << std::to_string(ping_ms) << " ms"
            std::cout << "-----------------------------------------------------\n"
            break;
        }
        case 'K':
        {
            auto rtt_file_end = std::chrono::high_resolution_clock::now();

            if (file_rtt_start == std::chrono::high_resolution_clock::time_point())
                break;

            double rtt_file_ms = std::chrono::duration<double, std::milli>(rtt_file_end - file_rtt_start).count();

            std::cout << "\n\n\n\n\n\n\n\n";
            std::cout << "-----------------------------------------------------\n"
            std::cout << "RTT FILE: " << rtt_file_ms << " ms\n";
            std::cout << "-----------------------------------------------------\n"
            std::cout << "\n\n\n\n\n\n\n\n";
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

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    
    std::string nick, t_buffer;

    while(true)
    {
        //system("clear");


        std::cout << "======= Chat log =======\n";
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
            std::cout << "3. File\n";
            std::cout << "4. Ping\n";
            std::cout << "5. Update Chat\n";
        }
            
        std::string buffer;
        std::cout << "Enter option: ";
        std::getline(std::cin, buffer);

        if (buffer == "1")
        {
            if (connected)
                continue;

            SocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

            do
            {
                std::cout << "Enter nickname: ";
                std::getline(std::cin, nick);
                
                auto t_rtt_start = std::chrono::high_resolution_clock::now();

                client.send_login(nick, SocketFD, stSockAddr);

                auto a_n_e = client.ack_or_error(SocketFD, stSockAddr);
                if (a_n_e == 'K')
                {
                    std::thread(reader_UDP, SocketFD).detach();
                    connected = true;

                    auto t_rtt_end = std::chrono::high_resolution_clock::now();
                    double rtt_ms = std::chrono::duration<double, std::milli>(t_rtt_end - t_rtt_start).count();
                    std::cout << "\n\n\n\n\n\n\n\n";
                    std::cout << "-----------------------------------------------------\n"
                    std::cout << "RTT: " << rtt_ms << " ms\n";
                    std::cout << "-----------------------------------------------------\n"
                    std::cout << "\n\n\n\n\n\n\n\n";
                }
                else
                    all_msgs.push_back("Server: Username already taken!");
                
            } while (!connected);
        }
        else if (buffer == "2")
        {
            if (connected)
            {
                client.send_logout(nick, SocketFD, stSockAddr);
                connected = false;
            }
            else
                std::cout << "Please login first!\n";
        }
        else if (buffer == "3")
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
                    continue;
                }

                file_rtt_start = std::chrono::high_resolution_clock::now();
                client.send_file(file_name, file, dst, nick, SocketFD, stSockAddr);
            }
            else
                std::cout << "Please login first!\n";
        }
        else if (buffer == "4")
            client.ping(SocketFD, stSockAddr);   
    }

    close(SocketFD);
    return 0;
}
