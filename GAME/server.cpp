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

#include "game.hpp"


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

   
   std::cout << "============ SERVER ============\n";
   while(true)
   {
      int ConnectFD = accept(SocketFD, NULL, NULL);
      
      while (true)
      {
         system("clear");
         
         TicTacToe game;
         game.print();
         while (true)
         {
            game = safe_read(ConnectFD);
            system("clear");
            game.print();

            if (!game.is_playing())
               break;

            game.play();
            send_game(ConnectFD, game);

            system("clear");
            game.print();
            
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
   }

   close(SocketFD);
   return 0;
}


