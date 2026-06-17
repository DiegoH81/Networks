#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>


#include "json.hpp"

// Auxiliar functions
std::string get_number(int in_number, int in_bytes)
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

std::string read_string(int in_socket, int in_length)
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

int read_number(int in_socket, int in_length)
{
   std::string num = read_string(in_socket, in_length);

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

namespace prt_send
{
    void ping(int in_socket)
    {
        write(in_socket, "P", 1);
    }

    void pong(int in_socket)
    {
        write(in_socket, "G", 1);
    }
   // Client to SV
   void login(std::string& in_nickname, int in_socket)
   {
      int in_bytes = 4;

      if (in_nickname.length() >= 1000)
         in_nickname.resize(999);

      std::string to_send = "L" + get_number(in_nickname.length(), in_bytes) + in_nickname;
      write(in_socket, to_send.c_str(), to_send.length());
   }
   
   void logout(int in_socket)
   {
      write(in_socket, "O", 1);
   }
   
   void file(std::string file_name, std::string file, std::string dest, int in_socket)
   {
      if(file.size() >= 1000000000000000)
         file.resize(999999999999999);

      if(file_name.size() >= 100000)
         file_name.resize(99999);

      if(dest.size() >= 100000)
         dest.resize(99999);

      int bytes = 5;
      int bytes_length_file = 15;

      std::string to_send = "F" + get_number(file.length(), bytes_length_file) + file + 
                                  get_number(file_name.length(), bytes) + file_name +
                                  get_number(dest.length(), bytes) + dest;
      
      write(in_socket, to_send.c_str(), to_send.length());
   }

   // SV to client

   void error(const std::string& in_msg, int in_socket)
   {
      int in_bytes = 5;

      auto msg = in_msg;
      if (msg.length() >= 100000)
         msg.resize(99999);

      std::string to_send = "E" + get_number(msg.length(), in_bytes) + msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void k_response(int in_socket)
   {
      write(in_socket, "K", 1);
   }

   
   void file_response(std::string file_name, std::string file, std::string src, int in_socket)
   {
      if(file.size() >= 1000000000000000)
         file.resize(999999999999999);

      if(file_name.size() >= 100000)
         file_name.resize(99999);

      if(src.size() >= 100000)
         src.resize(99999);

      int bytes = 5;
      int bytes_file = 15;

      std::string to_send = "f" + get_number(file.length(), bytes_file) + file + 
                                  get_number(file_name.length(), bytes) + file_name +
                                  get_number(src.length(), bytes) + src;
      
      write(in_socket, to_send.c_str(), to_send.length());
   }
   
}

namespace prt_recv
{
   // Client to SV
   std::string login(int in_socket)
   {
      int L = read_number(in_socket, 4);
      return read_string(in_socket, L);
   }

   std::string broadcast(int in_socket)
   {
      int L = read_number(in_socket, 7);
      return read_string(in_socket, L);
   }

   // Server to client

   bool k_response(int in_socket)
   {
      if (read_string(in_socket, 1) == "K")
         return true;
      return false;
   }

   std::string error(int in_socket)
   {
      int L = read_number(in_socket, 5);
      return read_string(in_socket, L);
   }

   void file_response(std::string& file_name, std::string& file, std::string& dest, int in_socket)
   {
      int L = read_number(in_socket, 15);
      file = read_string(in_socket, L);
      L = read_number(in_socket, 5);
      file_name = read_string(in_socket, L);
      L = read_number(in_socket, 5);
      dest = read_string(in_socket, L);

      std::string whole_pkt = "F" + get_number(L, 15) + file + get_number(L, 5) + file_name + get_number(L, 5) + dest;
      
      std::string first_500 = whole_pkt.substr(0, 500);
      std::string last_500  = whole_pkt.substr(whole_pkt.size() - 500);  
      std::cout << first_500 << "\n";
      std::cout << last_500 << "\n";
   }

}

#endif