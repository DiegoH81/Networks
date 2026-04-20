#ifndef GAME_H
#define GAME_H

#include <unistd.h>

#include <iostream>
#include <vector>

#define CROSS_PIECE 1
#define CIRCLE_PIECE 2


class TicTacToe
{
public:

// 1 = x, 2 = o
   int a,
       b,
       c,
       d,
       e,
       f,
       g,
       h,
       i,
       turn;
   char STATUS;

TicTacToe():
   a(0), b(0), c(0), d(0), e(0), f(0), g(0), h(0), i(0), turn(CROSS_PIECE), STATUS('N') // Status = N T O X
   {}


void print()
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   std::cout << "============ GAME ============\n";

   for (int i = 0; i < 3; i++)
   {
      std::cout << "-------\n";
      std::cout << "|";
      for (int j = 0; j < 3; j++)
      {
         int pos = 3 * i + j;
         if (*board[pos] == CROSS_PIECE)
            std::cout << "X";
         else if (*board[pos] == CIRCLE_PIECE)
            std::cout << "O";
         else
            std::cout << " ";
         std::cout << "|";
      }
      std::cout << "\n";
   }
   std::cout << "-------\n";
   std::cout << "Current turn: " << (turn == CROSS_PIECE? "X" : "O") << "\n";
   std::cout << "\n\nTOKEN LIST:\n";
   std::cout << "O\t-\tSERVER-CLIENT\n";
   std::cout << "X\t-\tCLIENT\n";
}

void play()
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};
   bool valid = false;
   int row = 0, col = 0;
   

   int pos = 0;
   
   while (!valid)
   {
      std::cout << "Enter Row: ";
      std::cin >> row;
      std::cout << "Enter Col: ";
      std::cin >> col;

      if (row < 0 || row >= 3 || col < 0 || col >= 3)
      {
         std::cout << "Invalid movement\n";
         continue;
      }
      
      pos = row * 3 + col;

      if (*board[pos] == 0)
      {
         *board[pos] = turn;
         valid = true;
      }
      else
         std::cout << "Box already used, pick another position!\n";
   }
   
   

   int cur_status = get_status();
   if (cur_status == 1)
      STATUS = 'X';
   else if (cur_status == 2)
      STATUS = 'O';
   else if (cur_status == 3)
      STATUS = 'T';

   if (turn == CROSS_PIECE)
      turn = CIRCLE_PIECE;
   else
      turn = CROSS_PIECE;
}

int get_status() // 0 None, 1 X, 2 O, 3 TIE
{
   for (int i = 0; i < 3; i++)
   {
      if (check_row(i, CROSS_PIECE) || check_col(i, CROSS_PIECE))
         return CROSS_PIECE;
      else if (check_row(i, CIRCLE_PIECE) || check_col(i, CIRCLE_PIECE))
         return CIRCLE_PIECE;
   }

   if (check_diagonal(CROSS_PIECE) || check_rev_diagonal(CROSS_PIECE))
      return CROSS_PIECE;
   else if (check_diagonal(CIRCLE_PIECE) || check_rev_diagonal(CIRCLE_PIECE))
      return CIRCLE_PIECE;
   else if (check_tie())
      return 3;

   return 0;
}

bool is_playing()
{
   switch (STATUS)
   {
   case 'X':
      std::cout << "CLIENT wins!\n";
      break;
   case 'O':
      std::cout << "SERVER-CLIENT wins!\n";
      break;
   case 'T':
      std::cout << "TIE!\n";
      break;
   default:
      return true;
      break;
   }

   return false;
}

bool check_row(int in_row, int in_value)
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   for (int i = 0; i < 3; i++)
      if (*board[in_row * 3 + i] != in_value)
         return false;

   return true;
}

bool check_col(int in_col, int in_value)
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   for (int i = 0; i < 3; i++)
      if (*board[(i * 3) + in_col] != in_value)
         return false;

   return true;
}

bool check_diagonal(int in_value)
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   for (int i = 0; i < 3; i++)
      if (*board[(i * 3) + i] != in_value)
         return false;

   return true;
}

bool check_rev_diagonal(int in_value)
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   for (int i = 0; i < 3; i++)
      if (*board[(i * 3) + (3 - i - 1)] != in_value)
         return false;

   return true;
}

bool check_tie()
{
   std::vector<int*> board = {&a, &b, &c, &d, &e, &f, &g, &h, &i};

   for (int i = 0; i < 9; i++)
      if (*board[i] == 0)
         return false;

   return true;
}

};

TicTacToe safe_read(int in_socket)
{
   TicTacToe to_ret;

   int total = 0;
   int n = 0;

   while (total < sizeof(TicTacToe))
   {
      n = read(in_socket, (char*)&to_ret + total, sizeof(TicTacToe) - total);
      
      if (n <= 0)
      {
         std::cout << "Error reading structure\n";
         break;
      }
      total += n;
   }
   
   return to_ret;
}

void send_game(int in_socket, TicTacToe &in_game)
{
   write(in_socket, &in_game, sizeof(in_game));
}


#endif