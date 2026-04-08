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

#include "protocols.h"

std::map<std::string, int> clients;


void reader(int in_socket)
{
  int n;
  std::string nick;
  // READ

  bool listening = true;
  while(listening)
  {
    char opt;
    read(in_socket, &opt, 1);

    switch (opt)
    {
    case 'L':
      nick = prot::R_CLI_SV_login(in_socket);
      clients[nick] = in_socket;
      prot::W_login_SV_CLI(in_socket);
      break;
    
    case 'B':
      auto data = prot::R_broadcast_CLI_SV(in_socket);
      for (auto &c : clients)
        prot::W_broadcast_SV_CLI(data, nick, in_socket);
      break;

    case 'U':
      auto data = prot::R_unicast_CLI_SV(in_socket);
      auto &msg = data.first;
      auto &in_nick = data.second;

      prot::W_unicast_SV_CLI(msg, nick, clients[in_nick]);
      break;
    
    case 'O':
      clients.erase(in_socket);
      listening = false;
      break;

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

  printf("----------SERVER----------\n");
  
  for(;;)
  {
    int ConnectFD = accept(SocketFD, NULL, NULL);
    std::thread(reader, ConnectFD).detach();
    
    
    std::cout << "\n\n\n\n";
    for (auto &c: clients)
        std::cout << c.first << " : " << c.second << "\n";
    std::cout << "\n\n\n\n";
  }

  close(SocketFD);
  return 0;
}


