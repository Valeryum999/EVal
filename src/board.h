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

namespace EVal {

#define setBit(bitboard, square) (bitboard |= C64(1) << square)
#define popBit(bitboard, square) (bitboard ^= C64(1) << square)

inline __attribute__((always_inline)) int getBit(Bitboard bitboard, unsigned int square){
    return bitboard >> square & 1;
}

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
    Bitboard pieceBoardCopy[12];                                         \
    Bitboard occupiedBoardCopy[3], emptyBoardCopy;                       \
    int butterflyBoardCopy[64];                                     \
    ColorType toMoveCopy;                                           \
    Square enPassantCopy;                                           \
    int castlingRightsCopy;                                         \
    Bitboard ZobristHashKeyCopy;                                         \
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
    Bitboard key;
    int depth;
    int flag;
    int bestMove;
    int evaluation;
} transpositionTableType;


class Board{
    Bitboard pieceBoard[12];
    Bitboard occupiedBoard[3];
    Bitboard emptyBoard;
    Piece mailboxBoard[64];
    int castlingRights;
    bool canEnPassant[2];
    Bitboard lastMovedPiece;
    Bitboard pawnAttacks[2][64];
    Bitboard knightAttacks[64];
    Bitboard bishopMasks[64];
    Bitboard bishopAttacks[64][512];
    Bitboard rookMasks[64];
    Bitboard rookAttacks[64][4096];
    Bitboard kingAttacks[64];
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
    Bitboard ZobristHashKey;
    int timeToThink;

    public:
        Board();
        constexpr void addMove(int move,moves *moveList);
        // unsigned int bitCount(Bitboard board) const;
        constexpr int evaluatePosition();
        Bitboard findMagicNumber(unsigned int square, int relevantBits, bool isBishop) const;
        void FromFEN(std::string FEN);
        constexpr Bitboard generateBishopAttacksOnTheFly(unsigned int square, Bitboard block) const;
        constexpr Bitboard generateRookAttacksOnTheFly(unsigned int square, Bitboard block) const;
        Bitboard generateMagicNumber() const;
        int generateRandomLegalMove();
        Bitboard generateZobristHashKey();
        constexpr Bitboard getBishopAttacks(unsigned int square, Bitboard occupancy) const;
        constexpr Bitboard getRookAttacks(unsigned int square, Bitboard occupancy) const;
        constexpr Bitboard getQueenAttacks(unsigned int square, Bitboard occupancy) const;
        int getAllPossibleMoves(moves *moveList);
        Piece getPieceAtSquare(Bitboard square) const;
        Bitboard getRandomBitboardnumber() const;
        void initLeaperPiecesMoves();
        void initMagicNumbers() const;
        constexpr void initTables();
        void initTranspositionTable();
        void initSliderPiecesMoves(bool bishop);
        bool isSquareAttacked(unsigned int square,ColorType color) const;
        int makeMove(int move);
        Bitboard maskKingAttacks(unsigned int square);
        Bitboard maskKnightAttacks(unsigned int square);
        Bitboard maskPawnAttacks(unsigned int square, ColorType color);
        Bitboard maskBishopOccupancy(unsigned int square) const;
        Bitboard maskRookOccupancy(unsigned int square) const;
        Square parseSquare(std::string square);
        int parseMove(std::string uciMove);
        void parseGo(std::string go);
        void parsePosition(std::string position);
        long long perftDriver(int depth);
        void perftTestSuite();
        void printBitBoard(Bitboard bitboard) const;
        void printMove(int move) const;
        void printMoveUCI(int move) const;
        void printMoveList(moves *moveList) const;
        void printMoveScores(moves *moveList, int *scores);
        int probeHash(int depth, int alpha, int beta, int *move);
        int quiescienceSearch(int alpha, int beta);
        void recordHash(int depth, int evaluation, int hashFlag, int move);
        int scoreMove(int move);
        int searchBestMove(int depth, int alpha, int beta);
        Bitboard setOccupancyBits(int index, int bitsInMask, Bitboard occupancy_mask) const;
        void sleep(int seconds);
        void sortMoves(moves *moveList);
        std::vector<std::string> split(std::string s,std::string delimiter);
        void startSearch();
        Bitboard swapNBits(Bitboard board, int i, int j, int n);
        void test();
        void UCImainLoop();
        int undoMove(int move);
        void visualizeBoard() const;
        void visualizeButterflyBoard() const;
        void ZobristHashingTestSuite();
};

constexpr bool is_ok(Square s) { return s >= a1 && s <= h8; }

constexpr Bitboard square_bb(Square s) {
    assert(is_ok(s));
    return (1ULL << s);
}

inline __attribute__((always_inline)) unsigned int popcount(Bitboard bitboard) {
    return __builtin_popcountll(bitboard);
}

inline __attribute__((always_inline)) unsigned int lsb(Bitboard bitboard) {
    return __builtin_ctzll(bitboard);
}

} //namespace EVal
#endif
