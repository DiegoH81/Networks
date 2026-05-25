#ifndef UDP_CLASS_H
#define UDP_CLASS_H

#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>

#include "aux_funcs.h"
#include "json.hpp"

class UDPProtocol
{
public:
    UDPProtocol():
        sequence_number(0), local_sequence_number(0), fragments(), total_expected()
    {}

    void send_message_udp(int in_socket, struct sockaddr_in& target_addr, std::string in_message)
    {
        int payload_size = 492;
        auto fragments = split_string(in_message, 492);

        int total_fragments = fragments.size();
        for (int i = 0; i < total_fragments; i++)
        {
            std::string total = get_number(total_fragments, 2);
            std::string count = get_number(i + 1, 2);
            std::string s_num = get_number(sequence_number, 4);

            std::string full_packet = total + count + s_num + fragments[i];

            full_packet.resize(500, '#');

            sendto(in_socket, full_packet.c_str(), 500, 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
        }

        sequence_number++;
    }

    void send_message_udp_AN(int in_socket, struct sockaddr_in& target_addr, char type,
                                                                             int total_fragments,
                                                                             int current_count)
    {
        int payload_size = 491;  

        std::string protocol = std::string(1, type);
        std::string total = get_number(total_fragments, 2);
        std::string count = get_number(current_count, 2);
        std::string s_num = get_number(sequence_number, 4);

        std::string full_packet = protocol + s_num + total + count;
        
        full_packet.resize(500, '#');

        sendto(in_socket, full_packet.c_str(), 500, 0, (struct sockaddr*)&target_addr, sizeof(target_addr));   

        sequence_number++;
    }

    std::string recv_fragment(int in_socket, struct sockaddr_in& sender_addr)
    {
        std::string to_return;

        char buf[500];
        socklen_t addr_len = sizeof(sender_addr);
        recvfrom(in_socket, buf, 500, 0, (struct sockaddr*)&sender_addr, &addr_len);

        std::string pkt(buf, 500);

        char fr = pkt.front();
        if (fr == 'K' || fr == 'N')
            return std::string(1, fr);


        int total = std::stoi(pkt.substr(0, 2));
        int count = std::stoi(pkt.substr(2, 2));
        int sq_n  = std::stoi(pkt.substr(4, 4));
        std::string payload = pkt.substr(8);


        total_expected[sq_n] = total;
        fragments[sq_n][count] = payload;

        if (fragments[sq_n].size() == total_expected[sq_n])
        {
            for (int i = 1; i <= total; i++)
                to_return += fragments[sq_n][i];
        }
        
        return to_return;
    }

    void add_sequence()
    {
        sequence_number++;
    }

protected:
    long int sequence_number, local_sequence_number;
    std::map<int, std::map<int, std::string>> fragments;
    std::map<int, int> total_expected;
};


class UDPClient : public UDPProtocol
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
                                    get_number(local_sequence_number, 12) +
                                    get_number(file.size(), 22) + file;

        to_send += std::string(1, get_checksum(to_send));
        send_message_udp(s_n, addr, to_send);
    }

    void send_login(std::string in_name, int s_n, struct sockaddr_in& addr)
    {
        if (in_name.size() >= 1000)
            in_name.resize(999);

        std::string to_send = "L" + get_number(in_name.length(), 4) + in_name + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_logout(std::string in_name, int s_n, struct sockaddr_in& addr)
    {
        if (in_name.size() >= 1000)
            in_name.resize(999);

        std::string to_send = "O" + get_number(in_name.length(), 4) + in_name + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_broadcast(std::string in_msg, std::string in_ori, int s_n, struct sockaddr_in& addr)
    {
        if (in_msg.size() >= 10000000)
            in_msg.resize(9999999);

        if (in_ori.size() >= 10000)
            in_ori.resize(9999);

        std::string to_send = "B" + get_number(in_msg.length(), 7) + in_msg + get_number(in_ori.length(), 4) + in_ori + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_unicast(std::string in_msg, std::string in_dest, std::string in_ori, int s_n, struct sockaddr_in& addr)
    {
        if (in_msg.size() >= 100000)
            in_msg.resize(99999);

        if (in_dest.size() >= 10000000)
            in_dest.resize(9999999);
        
        if (in_ori.size() >= 10000)
            in_ori.resize(9999);

        std::string to_send = "U" + get_number(in_msg.length(), 5) + in_msg + 
                                    get_number(in_dest.length(), 7) + in_dest +
                                    get_number(in_ori.length(), 4) + in_ori +
                                    get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_list(int s_n, struct sockaddr_in& addr)
    {
        std::string to_send = "T" + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }


    // Parse
    char ack_or_nack(int in_socket, struct sockaddr_in& addr)
    {
        auto msg = recv_fragment(in_socket, addr);
        if (msg.front() == 'K')
            return 'K';
        else if (msg.front() == 'N')
            return 'N';
        else
            return 'E';
    }

    bool parse_broadcast(std::string in_str, std::string& in_nick, std::string& in_msg)
    {
        std::string to_comp = "b" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;

        int l_nick = get_int_parse(in_str, 3);
        in_nick = parse_str(in_str, l_nick);
        int l_msg = get_int_parse(in_str, 7);
        in_msg = parse_str(in_str, l_msg);

        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 3 + l_nick + 7 + l_msg + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_unicast(std::string in_str, std::string& in_nick, std::string& in_msg)
    {
        std::string to_comp = "u" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;

        int l_nick = get_int_parse(in_str, 7);
        in_nick = parse_str(in_str, l_nick);
        int l_msg = get_int_parse(in_str, 5);
        in_msg = parse_str(in_str, l_msg);

        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 7 + l_nick + 5 + l_msg + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_list(std::string in_str, std::vector<std::string>& to_send)
    {
        std::string to_comp = "t" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;

        int l_list = get_int_parse(in_str, 5);
        std::string in_list = parse_str(in_str, l_list);

        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 5 + l_list + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);


        if (hash == new_hash)
        {

            nlohmann::json j = nlohmann::json::parse(in_list);

            for (auto& element : j.items())
                to_send.push_back(element.value());

            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }    

    bool parse_error(std::string in_str, std::string& in_error)
    {
        std::string to_comp = "E" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_error = get_int_parse(in_str, 5);
        in_error = parse_str(in_str, l_error);
        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 5 + l_error + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_file(std::string in_str, std::string& ori, std::string& dest, std::string& file_name, std::string& file)
    {
        std::string to_comp = "F" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_dest = get_int_parse(in_str, 5);
        

        dest = parse_str(in_str, l_dest);

        int l_file_name = get_int_parse(in_str, 3);
        file_name = parse_str(in_str, l_file_name);

        int l_ori = get_int_parse(in_str, 5);
        ori = parse_str(in_str, l_ori);

        long int seq_n = get_int_parse(in_str, 12);
        long int size_file = get_int_parse(in_str, 22);

        file = parse_str(in_str, size_file);


        char hash = parse_str(in_str, 1).front();

        total_size = total_size + 5 + l_dest + 3 + l_file_name + 5 + l_ori + 12 + 22 + size_file + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
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
                                    get_number(local_sequence_number, 12) +
                                    get_number(file.size(), 22) + file;

        to_send += std::string(1, get_checksum(to_send));
        send_message_udp(s_n, addr, to_send);
    }

    void send_error(std::string in_msg, int s_n, struct sockaddr_in& addr)
    {
        if (in_msg.size() >= 100000)
            in_msg.resize(99999);

        std::string to_send = "E" + get_number(in_msg.length(), 5) + in_msg + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_broadcast(std::string in_msg, std::string in_ori, int s_n, struct sockaddr_in& addr)
    {
        if (in_ori.size() >= 1000)
            in_ori.resize(999);

        if (in_msg.size() >= 10000000)
            in_msg.resize(9999999);


        std::string to_send = "b" + get_number(in_ori.length(), 3) + in_ori +  get_number(in_msg.length(), 7) + in_msg + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_unicast(std::string in_msg, std::string in_ori, int s_n, struct sockaddr_in& addr)
    {
        if (in_ori.size() >= 10000000)
            in_ori.resize(9999999);

        if (in_msg.size() >= 100000)
            in_msg.resize(99999);


        std::string to_send = "u" + get_number(in_ori.length(), 7) + in_ori +  get_number(in_msg.length(), 5) + in_msg + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_list(std::vector<std::string>& in_list, int s_n, struct sockaddr_in& addr)
    {
        nlohmann::json j;
        int counter = 1;

        for (auto &n: in_list)
        {
            j["Client_" + std::to_string(counter)] = n;
            counter++;
        }

        std::string list_str = j.dump();
        if (list_str.length() >= 100000)
            list_str.resize(99999);


        std::string to_send = "t" + get_number(list_str.length(), 5) + list_str + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_ack(int s_n, struct sockaddr_in& addr)
    {
        std::string to_send = "K" + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    void send_nack(int s_n, struct sockaddr_in& addr)
    {
        std::string to_send = "N" + get_number(local_sequence_number, 12);
        to_send += std::string(1, get_checksum(to_send));

        send_message_udp(s_n, addr, to_send);
    }

    // PARSE

    bool parse_file(std::string in_str, std::string& ori, std::string& dest, std::string& file_name, std::string& file)
    {
        std::string to_comp = "F" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_dest = get_int_parse(in_str, 5);
        

        dest = parse_str(in_str, l_dest);

        int l_file_name = get_int_parse(in_str, 3);
        file_name = parse_str(in_str, l_file_name);

        int l_ori = get_int_parse(in_str, 5);
        ori = parse_str(in_str, l_ori);

        long int seq_n = get_int_parse(in_str, 12);
        long int size_file = get_int_parse(in_str, 22);

        file = parse_str(in_str, size_file);


        char hash = parse_str(in_str, 1).front();

        total_size = total_size + 5 + l_dest + 3 + l_file_name + 5 + l_ori + 12 + 22 + size_file + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_login(std::string in_str, std::string& in_nick)
    {
        std::string to_comp = "L" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_nick = get_int_parse(in_str, 4);
        in_nick = parse_str(in_str, l_nick);
        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 4 + l_nick + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_logout(std::string in_str, std::string& in_nick)
    {
        std::string to_comp = "O" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_nick = get_int_parse(in_str, 4);
        in_nick = parse_str(in_str, l_nick);
        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 4 + l_nick + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_broadcast(std::string in_str, std::string& in_msg, std::string& in_ori)
    {
        std::string to_comp = "B" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;


        int l_msg = get_int_parse(in_str, 7);
        in_msg = parse_str(in_str, l_msg);

        int l_ori = get_int_parse(in_str, 4);
        in_ori = parse_str(in_str, l_ori);
        
        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = 7 + l_msg + 4 + l_ori + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }

    bool parse_unicast(std::string in_str, std::string& in_nick, std::string& in_msg, std::string &in_ori)
    {
        std::string to_comp = "U" + in_str.substr(0, in_str.size() - 1);
        long int total_size = 0;

        int l_msg = get_int_parse(in_str, 5);
        in_msg = parse_str(in_str, l_msg);
        int l_nick = get_int_parse(in_str, 7);
        in_nick = parse_str(in_str, l_nick);
        int l_ori = get_int_parse(in_str, 4);
        in_ori = parse_str(in_str, l_ori);

        long int seq_n = get_int_parse(in_str, 12);
        char hash = parse_str(in_str, 1).front();


        total_size = total_size + 5 + l_nick + 7 + l_msg + 4 + l_ori + 12 + 1;
        to_comp = to_comp.substr(0, total_size);

        char new_hash = get_checksum(to_comp);

        if (hash == new_hash)
        {
            local_sequence_number++;
            return true;
        }
        else 
            return false;
    }
};
#endif