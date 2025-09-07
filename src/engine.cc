#include <iostream>
#include "board.h"
#include <string>

char moveTypes[15][25] = {
    "QUIET_MOVE",
    "DOUBLE_PAWN_PUSH",
    "KING_CASTLE",
    "QUEEN_CASTLE",
    "CAPTURE",
    "EN_PASSANT_CAPTURE",
    "FILL",
    "FILL"
    "KNIGHT_PROMOTION",
    "BISHOP_PROMOTION",
    "ROOK_PROMOTION",
    "QUEEN_PROMOTION",
    "KNIGHT_PROMOTION_CAPTURE",
    "BISHOP_PROMOTION_CAPTURE",
    "ROOK_PROMOTION_CAPTURE",
    "QUEEN_PROMOTION_CAPTURE",
};

int main(){
    Board board = Board();
    //rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    //2n5/2pP4/1p1p4/2pP3r/KR2Pp1k/8/1p4P1/N7 w - c6 0 1
    //r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
    //8/8/8/8/8/8/8/8 w ---- -
    // board.FromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    // printf("Evaluation: %d\n", board.evaluatePosition());
    // printf("Best evaluation for white: %d\n", board.searchBestMove(4,-100000,100000));
    // printf("Evaluated %d positions\n",board.count);
    // printf("Best move: ");
    // board.printMove(board.bestMove);

    board.perftTestSuite();

    // for(int i=1; i<9; i++){
    //     auto start_time = std::chrono::high_resolution_clock::now();
    //     long long nodes = board.perftDriver(i,encodeMove(a1,a8,WhiteKing,WhiteBishop,1,1,1,1));
    //     auto end_time = std::chrono::high_resolution_clock::now();
    //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions Time: " << duration.count() << " milliseconds" << std::endl;
    // }
    
    return 0;
}