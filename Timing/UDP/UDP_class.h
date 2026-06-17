#ifndef UDP_CLASS_H
#define UDP_CLASS_H

#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "aux_funcs.h"
#include "json.hpp"

class UDPProtocol
{
public:
    UDPProtocol():
        chunk_order_length(2), seq_num_length(8), fragments()
    {}



    // UDP
    void send_message_udp(int in_socket, struct sockaddr_in& target_addr, std::string in_message)
    {

        int remaining_size = 500 - chunk_order_length - seq_num_length - 1;

        auto splitted_msg = split_string(in_message, remaining_size);
        
        int total_msgs = splitted_msg.size();

        std::cout << "TOTAL MSG TO SEND: " << total_msgs << "\n";

        for (int i = 0; i < total_msgs; i++)
        {
            std::string &current_string = splitted_msg[i];
            
            if (current_string.size() < remaining_size)
                current_string.resize(remaining_size, '#');

            std::string hash = std::string(1, get_checksum(current_string));
            std::string full_packet;
            std::string chunk_order = "00";
            
            if (total_msgs == 1) // 11 0000
            {
                full_packet = hash + "11" + get_number(0, seq_num_length) + current_string;

                //std::cout << "PKT SIZE sending: " << full_packet.size() << "\n";
                //print_pkt(full_packet, "SENDING");

                sendto(in_socket, full_packet.c_str(), 500, 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return;
            }
            else if (i == 0)
                chunk_order = "01";
            else if (i == total_msgs - 1)
                chunk_order = "11";
            
            full_packet = hash + chunk_order + get_number(i + 1, seq_num_length) + current_string;
            
            if (chunk_order == "01" || chunk_order == "11")
                print_pkt(full_packet, "SENDING");



            sendto(in_socket, full_packet.c_str(), 500, 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::string recv_fragment(int in_socket, struct sockaddr_in& sender_addr)
    {
        std::string in_paket(500, '\0');
        socklen_t addr_len = sizeof(sender_addr);
        recvfrom(in_socket, &in_paket[0], 500, 0, (struct sockaddr*)&sender_addr, &addr_len);

        std::string sender_key = std::string(inet_ntoa(sender_addr.sin_addr)) + ":" + std::to_string(ntohs(sender_addr.sin_port));

        std::string copy = in_paket;

        //std::cout << "PKT SIZE receieved: " << in_paket.size() << "\n";
        

        std::string hash = parse_str(in_paket, 1);
        std::string chunk_order = parse_str(in_paket, chunk_order_length);

        if (chunk_order == "01" || chunk_order == "11")
            print_pkt(in_paket, "RECEIEVING");

        int seq_number = get_int_parse(in_paket, seq_num_length);

        std::string new_hash(1, get_checksum(in_paket));

        if (hash != new_hash)
            std::cout << "Wrong hash at " << seq_number << "\n";


        //std::string sender_key = std::string(inet_ntoa(sender_addr.sin_addr)) + ":" + std::to_string(ntohs(sender_addr.sin_port));
        //std::cerr << "SENDER KEY: " << sender_key << "\n";

        if (chunk_order == "11" && seq_number == 0)
            return in_paket;

        fragments[sender_key][seq_number] = in_paket;


        if (chunk_order == "11") // FIN
        {
            std::string to_return;

            for (auto &piece : fragments[sender_key])
                to_return += piece.second;
            

            std::cout  << "RECEIEVED NUM: " << fragments[sender_key].size() << "\n";
            fragments.erase(sender_key);

            return to_return;
        }

        return "";
    }
protected:
    int chunk_order_length, seq_num_length;
    std::map<std::string, std::map<long int, std::string>> fragments;
};



class UDPClient : public UDPProtocol
{
public:
    std::chrono::high_resolution_clock::time_point ping_start;

    void send_file(std::string file_name, std::string file,
                   std::string dest, std::string ori,
                   int s_n, struct sockaddr_in& addr)
    {
        if (file.size() >= 1000000000000)
            file.resize(999999999999);
        if (file_name.size() >= 1000)
            file_name.resize(999);
        if (dest.size() >= 100000)
            dest.resize(99999);
        if (ori.size() >= 100000)
            ori.resize(99999);

        std::string to_send = "F" + get_number(dest.length(), 5) + dest +
                                    get_number(file_name.length(), 3) + file_name +
                                    get_number(ori.length(), 5) + ori +
                                    get_number(file.size(), 22) + file;

        send_message_udp(s_n, addr, to_send);
    }

    void send_login(std::string in_name, int s_n, struct sockaddr_in& addr)
    {
        if (in_name.size() >= 1000)
            in_name.resize(999);

        std::string to_send = "L" + get_number(in_name.length(), 4) + in_name;

        send_message_udp(s_n, addr, to_send);
    }

    void send_logout(std::string in_name, int s_n, struct sockaddr_in& addr)
    {
        if (in_name.size() >= 1000)
            in_name.resize(999);

        std::string to_send = "O" + get_number(in_name.length(), 4) + in_name;

        send_message_udp(s_n, addr, to_send);
    }

    

    // Parse
    char ack_or_error(int in_socket, struct sockaddr_in& addr)
    {
        auto msg = recv_fragment(in_socket, addr);
        if (msg.front() == 'K')
            return 'K';
        else
            return 'E';
    } 

    void parse_error(std::string in_str, std::string& in_error)
    {
        int l_error = get_int_parse(in_str, 5);
        in_error = parse_str(in_str, l_error);
    }

    void parse_file(std::string in_str, std::string& ori, std::string& dest, std::string& file_name, std::string& file)
    {
        std::cout << in_str.size() << "\n";

        int l_dest = get_int_parse(in_str, 5);
        dest = parse_str(in_str, l_dest);
        int l_file_name = get_int_parse(in_str, 3);
        file_name = parse_str(in_str, l_file_name);
        int l_ori = get_int_parse(in_str, 5);
        ori = parse_str(in_str, l_ori);
        long long int size_file = get_int_parse(in_str, 22);
        file = parse_str(in_str, size_file);
    }

    void ping(int s_n, struct sockaddr_in& addr)
    {
        ping_start = std::chrono::high_resolution_clock::now();
        
        send_message_udp(s_n, addr, "P");
    }
};


class UDPServer : public UDPProtocol
{
public:
    void send_file(std::string file_name, std::string file,
                   std::string dest, std::string ori,
                   int s_n, struct sockaddr_in& addr)
    {
        if (file.size() >= 1000000000000)
            file.resize(999999999999);
        if (file_name.size() >= 1000)
            file_name.resize(999);
        if (dest.size() >= 100000)
            dest.resize(99999);
        if (ori.size() >= 100000)
            ori.resize(99999);

        std::string to_send = "F" + get_number(dest.length(), 5) + dest +
                                    get_number(file_name.length(), 3) + file_name +
                                    get_number(ori.length(), 5) + ori +
                                    get_number(file.size(), 22) + file;

        send_message_udp(s_n, addr, to_send);
    }

    void send_error(std::string in_msg, int s_n, struct sockaddr_in& addr)
    {
        if (in_msg.size() >= 100000)
            in_msg.resize(99999);

        std::string to_send = "E" + get_number(in_msg.length(), 5) + in_msg;

        send_message_udp(s_n, addr, to_send);
    }

    void send_ack(int s_n, struct sockaddr_in& addr)
    {
        send_message_udp(s_n, addr, "K");
    }

    // PARSE
    void parse_file(std::string in_str, std::string& ori, std::string& dest, std::string& file_name, std::string& file)
    {
        std::string original_string = in_str;

        int l_dest = get_int_parse(in_str, 5);
        dest = parse_str(in_str, l_dest);
        int l_file_name = get_int_parse(in_str, 3);
        file_name = parse_str(in_str, l_file_name);
        int l_ori = get_int_parse(in_str, 5);
        ori = parse_str(in_str, l_ori);
        long long int size_file = get_int_parse(in_str, 22);
        file = parse_str(in_str, size_file);
    }

    void parse_login(std::string in_str, std::string& in_nick)
    {
        int l_nick = get_int_parse(in_str, 4);
        in_nick = parse_str(in_str, l_nick);
    }

    void parse_logout(std::string in_str, std::string& in_nick)
    {
        int l_nick = get_int_parse(in_str, 4);
        in_nick = parse_str(in_str, l_nick);
    }

    void send_pong(int s_n, struct sockaddr_in& addr)
    {
        send_message_udp(s_n, addr, "G");
    }
};
#endif