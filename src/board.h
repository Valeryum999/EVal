#ifndef BOARD_H
#define BOARD_H

#include "constants.h"
#include <iostream>
#include <vector>
#include <locale>
#include <chrono>
#include <bits/stdc++.h>
#include <cstdlib>
#include <thread>

#define bitCount(bitboard) __builtin_popcountll(bitboard)
#define getBit(bitboard, square) (bitboard >> square & 1)
#define setBit(bitboard, square) (bitboard |= C64(1) << square)
#define popBit(bitboard, square) (bitboard ^= C64(1) << square)

#define encodeMove(from, to, piece, promotedPiece, capture, doublePawnPush, enPassant, castling) \
    (from) |                \
    (to << 6) |             \
    (piece << 12) |         \
    (promotedPiece << 16) | \
    (capture << 20) |       \
    (doublePawnPush << 21)| \
    (enPassant << 22) |     \
    (castling << 23)

#define getFrom(move) (move & 0x3f)
#define getTo(move) ((move & 0xfc0) >> 6)
#define getPiece(move) ((move & 0xf000) >> 12)
#define getPromotedPiece(move) ((move & 0xf0000) >> 16)
#define getCaptureFlag(move) (move & 0x100000)
#define getDoublePawnPushFlag(move) (move & 0x200000)
#define getEnPassantFlag(move) (move & 0x400000)
#define getCastlingFlag(move) (move & 0x800000)

#define copyBoard()                                                 \
    U64 pieceBoardCopy[14];                                         \
    U64 occupiedBoardCopy, emptyBoardCopy;                          \
    ColorType toMoveCopy;                                           \
    Square enPassantCopy;                                           \
    int castlingRightsCopy;                                         \
    memcpy(pieceBoardCopy, pieceBoard, 112);                        \
    occupiedBoardCopy = occupiedBoard;                              \
    emptyBoardCopy = emptyBoard;                                    \
    toMoveCopy = toMove;                                            \
    enPassantCopy = enPassant;                                      \
    castlingRightsCopy = castlingRights;                            \

#define restoreBoard()                                              \
    memcpy(pieceBoard,pieceBoardCopy,112);                          \
    occupiedBoard = occupiedBoardCopy;                              \
    emptyBoard = emptyBoardCopy;                                    \
    toMove = toMoveCopy;                                            \
    enPassant = enPassantCopy;                                      \
    castlingRights = castlingRightsCopy;                            \

struct moves{
    int moves[256];
    int count;
};

class Board{
    U64 pieceBoard[14];
    U64 occupiedBoard;
    U64 emptyBoard;
    PieceType mailboxBoard[64];
    int castlingRights;
    bool canEnPassant[2];
    U64 lastMovedPiece;
    U64 pawnAttacks[2][64];
    U64 knightAttacks[64];
    U64 bishopMasks[64];
    U64 bishopAttacks[64][512];
    U64 rookMasks[64];
    U64 rookAttacks[64][4096];
    U64 kingAttacks[64];
    Square enPassant;
    ColorType toMove;
    int ply;
    int bestMove;
    int bestEval;
    int nodes;
    bool searchCancelled;

    public:
        Board();
        void addMove(int move,moves *moveList);
        unsigned int bitScanForward(U64 board) const;
        // unsigned int bitCount(U64 board) const;
        int evaluatePosition();
        U64 findMagicNumber(unsigned int square, int relevantBits, bool isBishop) const;
        void FromFEN(std::string FEN);
        U64 generateBishopAttacksOnTheFly(unsigned int square, U64 block) const;
        U64 generateRookAttacksOnTheFly(unsigned int square, U64 block) const;
        U64 generateMagicNumber() const;
        int generateRandomLegalMove();
        U64 getBishopAttacks(unsigned int square, U64 occupancy) const;
        U64 getRookAttacks(unsigned int square, U64 occupancy) const;
        U64 getQueenAttacks(unsigned int square, U64 occupancy) const;
        int getAllPossibleMoves(moves *moveList);
        PieceType getPieceTypeAtSquare(U64 square) const;
        U64 getRandomU64number() const;
        void initLeaperPiecesMoves();
        void initSliderPiecesMoves(bool bishop);
        void initMagicNumbers() const;
        bool isSquareAttacked(unsigned int square,ColorType color) const;
        int  makeMove(int move);
        U64 maskKingAttacks(unsigned int square);
        U64 maskKnightAttacks(unsigned int square);
        U64 maskPawnAttacks(unsigned int square, ColorType color);
        U64 maskBishopOccupancy(unsigned int square) const;
        U64 maskRookOccupancy(unsigned int square) const;
        Square parseSquare(std::string square);
        int parseMove(std::string uciMove);
        void parseGo(std::string go);
        void parsePosition(std::string position);
        long long perftDriver(int depth);
        void perftTestSuite();
        void printBitBoard(U64 bitboard) const;
        void printMove(int move) const;
        void printMoveUCI(int move) const;
        void printMoveList(moves *moveList) const;
        int searchBestMove(int depth, int alpha, int beta);
        U64 setOccupancyBits(int index, int bitsInMask, U64 occupancy_mask) const;
        void sleep(int seconds);
        std::vector<std::string> split(std::string s,std::string delimiter);
        void startSearch();
        U64 swapNBits(U64 board, int i, int j, int n);
        void test();
        void UCImainLoop();
        void visualizeBoard() const;
};
#endif
