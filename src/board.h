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
    U64 pieceBoardCopy[12];                                         \
    U64 occupiedBoardCopy[3], emptyBoardCopy;                       \
    int butterflyBoardCopy[64];                                     \
    ColorType toMoveCopy;                                           \
    Square enPassantCopy;                                           \
    int castlingRightsCopy;                                         \
    U64 ZobristHashKeyCopy;                                         \
    bool didPolyglotFlipEnPassantCopy;                              \
    memcpy(pieceBoardCopy, pieceBoard, 96);                         \
    memcpy(occupiedBoardCopy, occupiedBoard, 24);                   \
    memcpy(butterflyBoardCopy, butterflyBoard, 256);                \
    emptyBoardCopy = emptyBoard;                                    \
    toMoveCopy = toMove;                                            \
    enPassantCopy = enPassant;                                      \
    castlingRightsCopy = castlingRights;                            \
    ZobristHashKeyCopy = ZobristHashKey;                            \
    didPolyglotFlipEnPassantCopy = didPolyglotFlipEnPassant

#define restoreBoard()                                              \
    memcpy(pieceBoard,pieceBoardCopy,96);                           \
    memcpy(occupiedBoard, occupiedBoardCopy, 24);                   \
    memcpy(butterflyBoard, butterflyBoardCopy, 256);                \
    emptyBoard = emptyBoardCopy;                                    \
    toMove = toMoveCopy;                                            \
    enPassant = enPassantCopy;                                      \
    castlingRights = castlingRightsCopy;                            \
    ZobristHashKey = ZobristHashKeyCopy;                            \
    didPolyglotFlipEnPassant = didPolyglotFlipEnPassantCopy

struct moves{
    int moves[256];
    int count;
};

typedef struct {
    U64 key;
    int depth;
    int flag;
    int bestMove;
    int evaluation;
} transpositionTableType;


class Board{
    U64 pieceBoard[12];
    U64 occupiedBoard[3];
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
    int butterflyBoard[64];
    Square enPassant;
    bool didPolyglotFlipEnPassant;
    ColorType toMove;
    int ply;
    int maxPly;
    int PVLength[64];
    int PVTable[64][64];
    int bestMove;
    int bestEval;
    int killerMoves[2][64];
    int historyMoves[12][64];
    long long nodes;
    int searchDepth;
    bool searchCancelled;
    int mg_table[12][64];
    int eg_table[12][64];
    U64 ZobristHashKey;

    public:
        Board();
        void addMove(int move,moves *moveList);
        // unsigned int bitCount(U64 board) const;
        int evaluatePosition();
        U64 findMagicNumber(unsigned int square, int relevantBits, bool isBishop) const;
        void FromFEN(std::string FEN);
        U64 generateBishopAttacksOnTheFly(unsigned int square, U64 block) const;
        U64 generateRookAttacksOnTheFly(unsigned int square, U64 block) const;
        U64 generateMagicNumber() const;
        int generateRandomLegalMove();
        U64 generateZobristHashKey();
        U64 getBishopAttacks(unsigned int square, U64 occupancy) const;
        U64 getRookAttacks(unsigned int square, U64 occupancy) const;
        U64 getQueenAttacks(unsigned int square, U64 occupancy) const;
        int getAllPossibleMoves(moves *moveList);
        PieceType getPieceTypeAtSquare(U64 square) const;
        U64 getRandomU64number() const;
        void initLeaperPiecesMoves();
        void initMagicNumbers() const;
        void initTables();
        void initTranspositionTable();
        void initSliderPiecesMoves(bool bishop);
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
        void printMoveScores(moves *moveList, int *scores);
        int probeHash(int depth, int alpha, int beta);
        int quiescienceSearch(int alpha, int beta);
        void recordHash(int depth, int evaluation, int hashFlag);
        int scoreMove(int move);
        int searchBestMove(int depth, int alpha, int beta);
        U64 setOccupancyBits(int index, int bitsInMask, U64 occupancy_mask) const;
        void sleep(int seconds);
        void sortMoves(moves *moveList);
        std::vector<std::string> split(std::string s,std::string delimiter);
        void startSearch();
        U64 swapNBits(U64 board, int i, int j, int n);
        void test();
        void UCImainLoop();
        void visualizeBoard() const;
        void visualizeButterflyBoard() const;
        void ZobristHashingTestSuite();
};
#endif
