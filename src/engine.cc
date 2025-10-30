#include <iostream>
#include "board.h"
#include <string>

int main(int argc, const char *argv[]){
    bool isDebug = false;
    Board board = Board();
    if(argc == 2){
        if(!strcmp(argv[1],"DEBUG")) isDebug = true;
    }
    if(!isDebug) board.UCImainLoop();

    //debug

    board.perftTestSuite();
    
    return 0;
}