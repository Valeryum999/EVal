#ifndef UCI_H
#define UCI_H

#include <iostream>
#include "position.h"

namespace EVal {

namespace UCI {

void UCImainLoop(Position *pos);
Square parseSquare(std::string square);
int parseMove(Position *pos, std::string uciMove);
void parseGo(Position *pos, std::string go);
void parsePosition(Position *pos, std::string position);
void printMoveUCI(int move);

inline std::vector<std::string> split(std::string s,std::string delimiter){
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

}

}

#endif