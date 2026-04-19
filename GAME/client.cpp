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

#include "game.hpp"


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

   while (true)
   {
      system("clear");
      printf("============ CLIENT GAME ============\n");

      TicTacToe game;
      
      while (true)
      {
         send_game(SocketFD, game);
         game = safe_read(SocketFD);
         game.print();
         std::cout << "\n\n";
         
         if (!game.is_playing())
            break;
   
         
         //system("clear");
         
         
         game.play(CIRCLE_PIECE);
         
         game.print();
         std::cout << "\n\n";

         if (!game.is_playing())
            break;
      }

      int option;
      std::cout << "Do you want to play again?\n";
      std::cout << "1: YES, ANYTHING ELSE: NO:  ";
      std::cin >> option;

      if (option != 1)
         break;
   }
   

   close(SocketFD);
   return 0;
}
