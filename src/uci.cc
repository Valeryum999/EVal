#include "uci.h"
#include "movegen.h"

namespace EVal {
    
namespace UCI {

void UCImainLoop(Position *pos) {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    std::string input;

    std::cout << "id name EVal" << std::endl;
    std::cout << "id name Valeryum999" << std::endl;
    std::cout << std::endl;
    std::cout << "option name Debug Log File type string default" <<  std::endl;
    std::cout << "uciok" << std::endl;

    while(true){
        fflush(stdout);
        std::getline(std::cin, input);
        if(input == "") continue;
        if(input == "isready"){
            std::cout << "readyok" << std::endl;
            continue;
        }
        if(input.find("position") == 0){
            parsePosition(pos, input);
            continue;
        }
        if(input == "ucinewgame"){
            parsePosition(pos, "position startpos");
            continue;
        }
        if(input.find("go") == 0){
            parseGo(pos, input);
            continue;
        }
        if(input == "uci"){
            std::cout << "id name EVal" << std::endl;
            std::cout << "id name Valeryum999" << std::endl;
            std::cout << std::endl;
            std::cout << "option name Debug Log File type string default" <<  std::endl;
            std::cout << "uciok" << std::endl;
            continue;
        }
        if(input == "quit") break;
    }
}

void parseGo(Position *pos, std::string go){
    std::vector<std::string> command = split(go," ");
    // dynamic time thinking
    if(command.size() > 4){
        if(pos->to_move() == WHITE){
            pos->set_time_to_think(atoi(command[2].c_str()) / 60000 + 1);
        } else {
            pos->set_time_to_think(atoi(command[4].c_str()) / 60000 + 1);
        }
    } else {
        pos->set_time_to_think(5);
    }
    pos->startSearch();
    printf("\n");
    std::cout << "bestmove ";
    printMoveUCI(pos->best_move());
    std::cout << std::endl;
    //printMoveUCI(generateRandomLegalMove());
}

void parsePosition(Position *pos, std::string position){
    std::vector<std::string> command = split(position," ");
    if(command[1] == "startpos"){
        pos->FromFEN(startingPosition);
        if(command[2] == "moves") {
            for(auto it=command.begin()+3; it!=command.end(); it++){
                pos->makeMove(parseMove(pos, *it));
            }
        }
    } else if(command[1] == "fen"){
        int FENlength = position.find(command[8]) - position.find(command[2]);
        std::string FEN = position.substr(position.find(command[2]),FENlength);
        pos->FromFEN(FEN);
        if(command[8] == "moves"){
            for(auto it=command.begin()+9; it!=command.end(); it++){
                pos->makeMove(parseMove(pos, *it));
            }
        }
    }
}

Square parseSquare(std::string square){
    int file = square[0] - 'a';
    int rank = square[1] - '1';
    return (Square)(rank * 8 + file);
}

int parseMove(Position *pos, std::string uciMove) {
    moves moveList[1];
    MoveGen::getAllPossibleMoves(*pos, moveList);
    int length = uciMove.length();
    if(length != 4 && length != 5) return 0;
    unsigned int fromSquare = parseSquare(uciMove.substr(0,2));
    unsigned int toSquare = parseSquare(uciMove.substr(2,2));
    for(int i=0; i<moveList->count; i++){
        int currentMove = moveList->moves[i];
        if(getFrom(currentMove) == fromSquare && getTo(currentMove) == toSquare){
            if(length == 4) return currentMove;
            int promotedPiece = getPromotedPiece(currentMove);
            switch(uciMove[4]){
                case 'q':
                    if(promotedPiece == WhiteQueen || promotedPiece == BlackQueen) return currentMove;
                    break;
                case 'n':
                    if(promotedPiece == WhiteKnight || promotedPiece == BlackKnight) return currentMove;
                    break;
                case 'r':
                    if(promotedPiece == WhiteRook || promotedPiece == BlackRook) return currentMove;
                    break;
                case 'b':
                    if(promotedPiece == WhiteBishop || promotedPiece == BlackBishop) return currentMove;
                    break;
                default:
                    return 0;
            }
        }
    }
    return 0;
}

void printMoveUCI(int move) {
    std::cout << squares[getFrom(move)]
              << squares[getTo(move)];
    if(getPromotedPiece(move))
        std::cout << promotedPieces[getPromotedPiece(move)];
}

} // namespace UCI

} // namespace EVal