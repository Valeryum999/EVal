#include "constants.h"

namespace EVal{

std::string emptyBoard = "8/8/8/8/8/8/8/8 w - - ";
std::string startingPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
std::string trickyPosition = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ";
std::string killerPosition = "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1";
std::string cmkPosition = "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 ";

std::string unicodePieces[12] = {
    u8"♟",
    u8"♞",
    u8"♝",
    u8"♜",
    u8"♛",
    u8"♚",
    u8"♙",
    u8"♘",
    u8"♗",
    u8"♖",
    u8"♕",
    u8"♔"
};

std::string whitePawn = u8"♟";
std::string whiteKing = u8"♚";
std::string whiteQueen = u8"♛";
std::string whiteRook = u8"♜";
std::string whiteBishop = u8"♝";
std::string whiteKnight = u8"♞";
std::string blackKing = u8"♔";
std::string blackQueen = u8"♕";
std::string blackRook = u8"♖";
std::string blackKnight = u8"♘";
std::string blackBishop = u8"♗";
std::string blackPawn = u8"♙";

int mg_table[12][64];
int eg_table[12][64];

} // namespace EVal