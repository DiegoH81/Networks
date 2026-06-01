#ifndef AUX_FUNCS_H
#define AUX_FUNCS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <fstream>

// Auxiliar functions
std::string get_number(long int in_number, int in_bytes)
{
    std::string to_return(in_bytes, '0');

    std::string num = std::to_string(in_number);


    int eo_in_num = num.length() - 1;
    int eo_return = in_bytes - 1;

    for (int i = 0; i < num.length(); i++)
    {
        to_return[eo_return] = num[eo_in_num];
        eo_return--; eo_in_num--;
    }
        
    return to_return;
}


// TCP
std::string read_string_TCP(int in_socket, int in_length)
{
    std::string to_return;
    int n;
    to_return.resize(in_length);

    int total = 0;

    while (total < in_length)
    {
        n = read(in_socket, &to_return[total], in_length - total);

        if (n <= 0)
            break;
            
        total += n;
    }

    return to_return;
}

int read_number_TCP(int in_socket, int in_length)
{
    std::string num = read_string_TCP(in_socket, in_length);

    return std::stoi(num);
}

// UDP
std::string read_string_UDP(int in_socket, int in_length = 500)
{
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    std::string to_return;
    to_return.resize(in_length);
   
    int n = recvfrom(in_socket, &to_return[0], in_length, 0, (struct sockaddr*)&from_addr, &addr_len);
    return to_return;
}

int read_number_from_STR(std::string& in_msg, int start, int end)
{
    std::string num;

    for (int i = start; i < end; i++)
        num += in_msg[i];
        
    return std::stoi(num);
}




std::string read_binary_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
        return ""; 

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(size);


    if (file.read(&buffer[0], size))
        return buffer;

    return "";
}

bool write_binary_file(const std::string& filename, const std::string& data)
{
    std::ofstream file(filename, std::ios::binary | std::ios::out | std::ios::trunc);

    if (!file.is_open())
        return false;

    if (file.write(data.data(), data.size()))
        return true;

    return false;
}

char get_checksum(std::string to_hash)
{
    int total_sum = 0;

    for (auto &c : to_hash)
        total_sum += c;
    
    total_sum %= 7;

    return total_sum;
}

std::vector<std::string> split_string(std::string in_str, int batch_size)
{
    std::vector<std::string> to_return;

    size_t pos = 0;

    while (pos + batch_size < in_str.size())
    {
        to_return.push_back(in_str.substr(pos, batch_size));        
        pos += batch_size;
    }


    if (pos < in_str.size())
        to_return.push_back(in_str.substr(pos));


    return to_return;
}

std::string parse_str(std::string& in_str, int n)
{
    std::string to_return = in_str.substr(0, n);
    in_str = in_str.substr(n);

    return to_return;
}

int get_int_parse(std::string& in_str, int n)
{
    std::string num = parse_str(in_str, n);

    return std::stoi(num);
}

void print_pkt(std::string in_paket, std::string msg)
{
    //char print_buf[501];
    //memset(print_buf, 0, 501);
    //memcpy(print_buf, in_paket.c_str(), 500);

    std::cerr << msg << " << ";
    
    for (unsigned char c : in_paket)
    {
         if (c >= 32 && c < 127)
            std::cerr << c;
        else
            std::cerr << "?";// << int(c);
    }

    std::cerr << " >>" << std::endl;
}

#endif