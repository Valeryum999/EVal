#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

typedef unsigned long long U64; // supported by MSC 13.00+ and C99 
#define C64(constantU64) constantU64##ULL


enum ColorType{
    White,
    Black,
};

enum PieceType{
    WhitePawn = 2,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
};

enum Square {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    noSquare
};

enum Ranks{
    FIRST_RANK,
    SECOND_RANK,
    THIRD_RANK,
    FOURTH_RANK,
    FIFTH_RANK,
    SIXTH_RANK,
    SEVENTH_RANK,
    EIGHT_RANK,
};

enum Files{
    A_FILE,
    B_FILE,
    C_FILE,
    D_FILE,
    E_FILE,
    F_FILE,
    G_FILE,
    H_FILE,
};

extern U64 FILES[8];
extern U64 RANKS[8];
extern U64 notAFile;
extern U64 notHFile;
extern U64 notFirstRank;
extern U64 notEighthRank;
extern U64 notFirstRankAFile;
extern U64 notFirstRankHFile;
extern U64 notEighthRankAFile;
extern U64 notEighthRankHFile;
extern U64 notABFiles;
extern U64 notGHFiles;
extern U64 DIAGONAL;
extern U64 ANTIDIAGONAL;
extern U64 LIGHT_SQUARES;
extern U64 DARK_SQUARES;

extern const int index64[64];

extern const int bishopRelevantBits[64];
extern const int rookRelevantBits[64];

extern const U64 bishopMagicNumbers[64];
extern const U64 rookMagicNumbers[64];

extern const char squares[65][3];
extern const char pieces[14][12];
extern const char promotedPieces[];
extern std::string unicodePieces[14];
extern std::string whitePawn;
extern std::string whiteKing;
extern std::string whiteQueen;
extern std::string whiteRook;
extern std::string whiteBishop;
extern std::string whiteKnight;
extern std::string blackKing;
extern std::string blackQueen;
extern std::string blackRook;
extern std::string blackKnight;
extern std::string blackBishop;
extern std::string blackPawn;

extern std::string startingPosition;

extern const int castlingRightsTable[64];

#endif