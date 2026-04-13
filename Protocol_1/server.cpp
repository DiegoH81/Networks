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
    {
      nick = prt_recv::login(in_socket);

      if (clients.find(nick) != clients.end())
      {
        prt_send::error("Username already taken!", in_socket);
        break;
      }

      clients[nick] = in_socket;
      prt_send::k_response(in_socket);

      std::cout << "\n";
      for (auto& i: clients)
        std::cout << i.first << " - " << i.second <<"\n";

      break;
    }
    case 'B':
    {
      auto data = prt_recv::broadcast(in_socket);
      for (auto &c : clients)
        prt_send::broadcast_response(data, nick, c.second);
      break;
    }
    case 'U':
    {
      auto data = prt_recv::unicast(in_socket);
      auto &msg = data.first;
      auto &in_nick = data.second;

      prt_send::unicast_response(msg, nick, clients[in_nick]);
      break;
    }
    case 'T':
    {
      std::vector<std::string> clients_vec;

      for (auto &c : clients)
        clients_vec.push_back(c.first);

      prt_send::list_response(clients_vec, clients[nick]);

      break;
    }
    case 'F':
    {
      std::string file, file_name, dest;

      prt_recv::file_response(file_name, file, dest, in_socket);
      

      prt_send::file_response(file_name, file, nick, clients[dest]);

      break;
    }
    case 'O':
    {
      prt_send::k_response(in_socket);
      clients.erase(nick);
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

  printf("----------SERVER----------\n");
  
  while(true)
  {
    int ConnectFD = accept(SocketFD, NULL, NULL);
    std::thread(reader, ConnectFD).detach();
  }

  close(SocketFD);
  return 0;
}


