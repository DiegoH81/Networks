#include <unistd.h>

#include <string>
#include <vector>


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
   char buffer[2000];
   int n;
   n = read(in_socket, buffer, in_length);
      buffer[n] = '\0';

   return std::string(buffer);
}

int read_number(int in_socket, int in_length)
{
   std::string num = read_string(in_socket, in_length);

   return std::stoi(num);
}


namespace prot
{
   void W_login_CLI_SV(const std::string& in_nickname, int in_socket)
   {
      int in_bytes = 3;
      std::string to_send = "L" + get_number(in_nickname.length(), in_bytes) + in_nickname;
      write(in_socket, to_send.c_str(), to_send.length());
   }
   void W_login_SV_CLI(int in_socket)
   {
      write(in_socket, "K", 1);
   }

   void W_logout_CLI_SV(int in_socket)
   {
      write(in_socket, "O", 1);
   }

   void W_error(const std::string& in_msg, int in_socket)
   {
      int in_bytes = 5;
      std::string to_send = "E" + get_number(in_msg.length(), in_bytes) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void W_broadcast_CLI_SV(const std::string& in_msg, int in_socket)
   {
      int in_bytes = 7;
      std::string to_send = "B" + get_number(in_msg.length(), in_bytes) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void W_broadcast_SV_CLI(const std::string& in_msg, const std::string& in_nick, int in_socket)
   {
      int bytes_nick = 3;
      int bytes_msg = 7;

      std::string to_send = "b" + get_number(in_nick.length(), bytes_nick) + in_nick + 
                                  get_number(in_msg.length(), bytes_msg) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void W_unicast_CLI_SV(const std::string& in_msg, const std::string& in_nick, int in_socket)
   {
      int bytes_msg = 5;
      int bytes_nick = 7;

      std::string to_send = "U" + get_number(in_msg.length(), bytes_msg) + in_msg + 
                                  get_number(in_nick.length(), bytes_nick) + in_nick;
      write(in_socket, to_send.c_str(), to_send.length());
   }

   void W_unicast_SV_CLI(const std::string& in_msg, const std::string& in_nick, int in_socket)
   {
      int bytes_msg = 5;
      int bytes_nick = 7;

      std::string to_send = "u" + get_number(in_nick.length(), bytes_nick) + in_nick +
                                  get_number(in_msg.length(), bytes_msg) + in_msg;
      write(in_socket, to_send.c_str(), to_send.length());
   }


// READ
   std::string R_CLI_SV_login(int in_socket)
   {
      int L = read_number(in_socket, 3);
      return read_string(in_socket, L);
   }

   bool R_SV_CLI_login(int in_socket)
   {
      if (read_string(in_socket, 1) == "K")
         return true;
      return false;
   }

   std::string R_error(int in_socket)
   {
      int L = read_number(in_socket, 5);
      return read_string(in_socket, L);
   }

   std::string R_broadcast_CLI_SV(int in_socket)
   {
      int L = read_number(in_socket, 7);
      return read_string(in_socket, L);
   }

   std::pair<std::string, std::string> R_broadcast_SV_CLI(int in_socket)
   {
      int L = read_number(in_socket, 3);
      auto nick = read_string(in_socket, L);
      L = read_number(in_socket, 7);
      auto msg = read_string(in_socket, L);

      return {nick, msg};
   }

   std::pair<std::string, std::string> R_unicast_CLI_SV(int in_socket)
   {
      int L = read_number(in_socket, 5);
      auto msg = read_string(in_socket, L);
      L = read_number(in_socket, 7);
      auto nick = read_string(in_socket, L);

      return {msg, nick};
   }

   std::pair<std::string, std::string> R_unicast_SV_CLI(int in_socket)
   {
      int L = read_number(in_socket, 7);
      auto nick = read_string(in_socket, L);
      L = read_number(in_socket, 5);
      auto msg = read_string(in_socket, L);

      return {nick, msg};
   }
}