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

#include "protocols.h"

bool connected = false;

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
         auto data = prt_recv::broadcast_response(in_socket);
         std::string msg_to_push = data.first + ": " + data.second;
         all_msgs.push_back(msg_to_push);

         break;
      }
      case 'u':
      {
         auto data = prt_recv::unicast_response(in_socket);
         std::string msg_to_push = data.first + ": " + data.second;
         all_msgs.push_back(msg_to_push);
         break;
      }
      case 't':
      {  
         auto clients_list = prt_recv::list_response(in_socket);

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
         prt_recv::file_response(file_name, file, ori, in_socket);

         write_binary_file("New_" + file_name , file);

         break;
      }
      case 'E':
      {
         std::string error = prt_recv::error(in_socket);

         std::string msg_to_push = "SERVER: " + error;
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


int main(void)
{
   struct sockaddr_in stSockAddr;
   int Res;
   int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   int n;

   memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

   stSockAddr.sin_family = AF_INET;
   stSockAddr.sin_port = htons(45000);
   Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);


   std::string nick;   

   while(true)
   {
      system("clear");

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

         SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
         connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

         do
         {
            std::cout << "Enter nickname: ";
            std::getline(std::cin, nick);
      
            prt_send::login(nick, SocketFD);
            connected = prt_recv::k_response(SocketFD);
            
            if (connected)
               std::thread(reader, SocketFD).detach();
            else
            {
               std::string error = prt_recv::error(SocketFD);
               std::cout << "\nSERVER: " + error << "\n";
            }

         } while (!connected);
         
         
         break;
      }
      case 2:
      {
         if (connected)
         {
            prt_send::logout(SocketFD);
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
      
            prt_send::broadcast(msg, SocketFD);
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
      
            prt_send::unicast(msg, dst, SocketFD);
         }
         else
            std::cout << "Please login first!\n";
         break;
      }
      case 5:
      {
         if (connected)
            prt_send::list(SocketFD);
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
      
            prt_send::file(file_name, file, dst, SocketFD);
         }
         else
            std::cout << "Please login first!\n";
         break;
      }
      default:
         break;
      }
   }

   return 0;
}
