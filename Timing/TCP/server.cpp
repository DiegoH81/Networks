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
#include <chrono>

#include "protocols.h"

std::map<std::string, int> clients;
std::mutex clients_mutex;

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
         if (clients.find(nick) != clients.end())
            clients.erase(nick);
         lock.unlock();

         break;
      }

      switch (protocol)
      {
      case 'L':
      {
         nick = prt_recv::login(in_socket);

         std::unique_lock<std::mutex> lock(clients_mutex);
         if (clients.find(nick) != clients.end())
         {
            lock.unlock();
            prt_send::error("Username already taken!", in_socket);
            break;
         }

         clients[nick] = in_socket;
         lock.unlock();

         prt_send::k_response(in_socket);
         
         std::unique_lock<std::mutex> lock_B(clients_mutex);
         
         std::cout << "\n";
         for (auto& i: clients)
            std::cout << "C: " << i.first << " - Socket: " << i.second <<"\n";
         lock_B.unlock();

         break;
      }
      case 'F':
      {
         auto t_start = std::chrono::high_resolution_clock::now();


         std::string file, file_name, dest;

         prt_recv::file_response(file_name, file, dest, in_socket);
         
         std::unique_lock<std::mutex> lock(clients_mutex);

         if (clients.find(dest) == clients.end())
         {
            prt_send::error("User not found!", in_socket);
            lock.unlock();
            break;
         }

         prt_send::file_response(file_name, file, nick, clients[dest]);
         lock.unlock();

         auto t_end = std::chrono::high_resolution_clock::now();
         double processing_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
         std::cout << "NODAL processing time: " << processing_ms << " ms\n";

         break;
      }
      case 'O':
      {
         prt_send::k_response(in_socket);
         std::unique_lock<std::mutex> lock(clients_mutex);
         clients.erase(nick);

         std::cout << "\n";
         for (auto& i: clients)
            std::cout << "C: " << i.first << " - Socket: " << i.second <<"\n";
         lock.unlock();
         
         listening = false;
         break;
      }
      case 'P':
      {
          prt_send::pong(in_socket);
          break;
      }
         default:
         break;
      }
  
   }

   close(in_socket);
}


int main(void)
{
   struct sockaddr_in stSockAddr;
   int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   std::string buffer;
   int n;

   memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

   stSockAddr.sin_family = AF_INET;
   stSockAddr.sin_port = htons(45000);
   stSockAddr.sin_addr.s_addr = INADDR_ANY;

   bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
   listen(SocketFD, 10);

   printf("========= SERVER TERMINAL =========\n");
  
   while(true)
   {
      int ConnectFD = accept(SocketFD, NULL, NULL);
      std::thread(reader, ConnectFD).detach();
   }

   close(SocketFD);
   return 0;
}


