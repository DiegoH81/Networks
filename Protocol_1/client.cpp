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
      auto data = prot::R_broadcast_SV_CLI(in_socket);
      std::cout << data.first << ": " << data.second << "\n";  
      break;
    
    case 'u':
      auto data = prot::R_unicast_SV_CLI(in_socket);
      std::cout << data.first << ": " << data.second << "\n";  
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

    int option = 0;
    std::cin >> option;

    switch (option)
    {
    case 1:
      std::string nick;
      std::cout << "Enter nickname: ";
      std::getline(std::cin, nick);

      prot::W_login_CLI_SV(nick, SocketFD);
      if (prot::R_SV_CLI_login(SocketFD))
        std::thread(reader, SocketFD).detach();

      break;

    case 2:
      prot::W_logout_CLI_SV(SocketFD);
      connected = false;
      break;

    case 3:
      std::string msg;
      std::cout << "Enter message: ";
      std::getline(std::cin, msg);

      prot::W_broadcast_CLI_SV(msg, SocketFD);
      break;

    case 4:
      std::string dst;
      std::cout << "Enter destinatary: ";
      std::getline(std::cin, dst);

      std::string msg;
      std::cout << "Enter message: ";
      std::getline(std::cin, msg);

      prot::W_unicast_CLI_SV(msg, dst, SocketFD);
      break;

    default:
      break;
    }
  }

  shutdown(SocketFD, SHUT_RDWR);
  close(SocketFD);
  return 0;
}
