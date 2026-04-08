#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <unistd.h>

#include <string>
#include <vector>

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

std::string read_string(int in_socket, int in_length, int in_buffer_length)
{
   char *buffer = new char[in_buffer_length];
   int n;
   n = read(in_socket, buffer, in_length);
      buffer[n] = '\0';
      
   std::string to_return (buffer);
   delete[] buffer;
   return to_return;
}

int read_number(int in_socket, int in_length)
{
   std::string num = read_string(in_socket, in_length, in_length);

   return std::stoi(num);
}


namespace prt_send
{
   // Client to SV
   void login(std::string& in_nickname, int in_socket)
   {
      int in_bytes = 3;

      if (in_nickname.length() >= 1000)
         in_nickname.resize(999);

      std::string to_send = "L" + get_number(in_nickname.length(), in_bytes) + in_nickname;
      write(in_socket, to_send.c_str(), to_send.length());
   }
   
   void logout(int in_socket)
   {
      write(in_socket, "O", 1);
   }
   
   void broadcast(std::string& in_msg, int in_socket)
   {
      int in_bytes = 7;

      if (in_msg.length() >= 10000000)
         in_msg.resize(9999999);

      std::string to_send = "B" + get_number(in_msg.length(), in_bytes) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void unicast(std::string& in_msg, std::string& in_nick, int in_socket)
   {
      int bytes_msg = 5;
      int bytes_nick = 7;

      if (in_nick.length() >= 10000000)
         in_nick.resize(9999999);

      if (in_msg.length() >= 100000)
         in_msg.resize(99999);

      std::string to_send = "U" + get_number(in_msg.length(), bytes_msg) + in_msg + 
                                  get_number(in_nick.length(), bytes_nick) + in_nick;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   // SV to client

   void error(std::string& in_msg, int in_socket)
   {
      int in_bytes = 5;

      if (in_msg.length() >= 100000)
         in_msg.resize(99999);

      std::string to_send = "E" + get_number(in_msg.length(), in_bytes) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void login_response(int in_socket)
   {
      write(in_socket, "K", 1);
   }

   void broadcast_response(std::string& in_msg, std::string& in_nick, int in_socket)
   {
      int bytes_nick = 3;
      int bytes_msg = 7;

      if (in_nick.length() >= 1000)
         in_nick.resize(999);

      if (in_msg.length() >= 10000000)
         in_msg.resize(9999999);

      std::string to_send = "b" + get_number(in_nick.length(), bytes_nick) + in_nick + 
                                  get_number(in_msg.length(), bytes_msg) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void unicast_response(std::string& in_msg, std::string& in_nick, int in_socket)
   {
      int bytes_msg = 5;
      int bytes_nick = 7;

      if (in_nick.length() >= 10000000)
         in_nick.resize(9999999);

      if (in_msg.length() >= 100000)
         in_msg.resize(99999);

      std::string to_send = "u" + get_number(in_nick.length(), bytes_nick) + in_nick +
                                  get_number(in_msg.length(), bytes_msg) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }
}

namespace prt_recv
{
   // Client to SV
   std::string login(int in_socket)
   {
      int L = read_number(in_socket, 3);
      return read_string(in_socket, L, 2000);
   }

   std::string broadcast(int in_socket)
   {
      int L = read_number(in_socket, 7);
      return read_string(in_socket, L, 2000);
   }

   std::pair<std::string, std::string> unicast(int in_socket)
   {
      int L = read_number(in_socket, 5);
      auto msg = read_string(in_socket, L, 2000);
      L = read_number(in_socket, 7);
      auto nick = read_string(in_socket, L, 2000);

      return {msg, nick};
   }

   // Server to client

   bool login_response(int in_socket)
   {
      if (read_string(in_socket, 1, 1) == "K")
         return true;
      return false;
   }

   std::string error(int in_socket)
   {
      int L = read_number(in_socket, 5);
      return read_string(in_socket, L, 2000);
   }

   std::pair<std::string, std::string> broadcast_response(int in_socket)
   {
      int L = read_number(in_socket, 3);
      auto nick = read_string(in_socket, L, 2000);
      L = read_number(in_socket, 7);
      auto msg = read_string(in_socket, L, 2000);

      return {nick, msg};
   }

   std::pair<std::string, std::string> unicast_response(int in_socket)
   {
      int L = read_number(in_socket, 7);
      auto nick = read_string(in_socket, L, 2000);
      L = read_number(in_socket, 5);
      auto msg = read_string(in_socket, L, 2000);

      return {nick, msg};
   }
}

#endif