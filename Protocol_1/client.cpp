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

#include "protocols.h"

bool connected = true;

void reader(int in_socket)
{
  int n;
  // READ

  while(connected)
  {
    char opt;
    read(in_socket, &opt, 1);

    switch (opt)
    {
    case 'b':
    {

      auto data = prt_recv::broadcast_response(in_socket);
      std::cout << data.first << ": " << data.second << "\n";  
      break;
    }
    case 'u':
    {
      auto data = prt_recv::unicast_response(in_socket);
      std::cout << data.first << ": " << data.second << "\n";  
      break;
    }
    case 't':
    { 
      auto clients_list = list_response(in_socket);

      std::cout << "LIST\n";
      for(auto &c : clients_list)
        std::cout << c << "\n";

        std::cout << "\n";

      break;
    }
    case 'f':
    {
      std::string& file_name, file, ori;
      prt_recv(file_name, file, ori, in_socket);

      std::cout << ori << ": " << file_name << ": " << file << "\n";

      break;
    }
    default:
      break;
    }

  }
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

  connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

  

  while(connected)
  {
    std::cout << "MENU\n";
    std::cout << "1. Login\n";
    std::cout << "2. Logout\n";
    std::cout << "3. Broadcast\n";
    std::cout << "4. Unicast\n";
    std::cout << "5. List\n";
    std::cout << "6. File\n";

    std::string buffer;
    std::cout << "Enter option: ";
    std::getline(std::cin, buffer);

    int option = std::stoi(buffer);

    switch (option)
    {
    case 1:
    {
      std::string nick;
      std::cout << "Enter nickname: ";
      std::getline(std::cin, nick);

      prt_send::login(nick, SocketFD);
      if (prt_recv::login_response(SocketFD))
        std::thread(reader, SocketFD).detach();

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

  shutdown(SocketFD, SHUT_RDWR);
  close(SocketFD);
  return 0;
}
