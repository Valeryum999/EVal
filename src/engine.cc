#include <iostream>
#include "board.h"
#include <string>

int main(){
    Board board = Board();
    board.FromFEN("rnbqkbnr/pPpppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.visualizeBoard();

    std::string input;
    while(1){
        std::cout << "Make a move: ";
        std::cin >> input;
        board.printMove(board.parseMove(input));
        std::cout << "Printed move" << std::endl;
    }
    return 0;
}