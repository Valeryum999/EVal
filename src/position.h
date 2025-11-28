#ifndef BOARD_H
#define BOARD_H

#include "constants.h"
#include "types.h"
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

inline __attribute__((always_inline)) int getBit(Bitboard bitboard, Square square){
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
    Color toMoveCopy;                                           \
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


class Position{
    Bitboard pieceBoard[12];
    Bitboard occupiedBoard[3];
    Bitboard emptyBoard;
    Piece mailboxBoard[64];
    int castlingRights;
    bool canEnPassant[2];
    Bitboard lastMovedPiece;
    int butterflyBoard[64];
    Square enPassant;
    bool didPolyglotFlipEnPassant;
    Color toMove;
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
        Position();

        // Position representation
        Bitboard pieces() const; // get all pieces (== occupiedBoard)
        template<typename... PieceType>
        Bitboard pieces(PieceType... piece) const;
        Bitboard pieces(Color c) const;
        template<typename... PieceType>
        Bitboard pieces(Color c, PieceType... piece) const;
        Piece piece_on(Square square) const;
        Color to_move() const;
        int castling_rights() const;
        Square ep_square() const;
        bool isSquareAttacked(Square square, Color color) const;

        //search and evaluation
        int makeMove(int move);
        int scoreMove(int move);
        void initTables();
        int quiescienceSearch(int alpha, int beta);
        int searchBestMove(int depth, int alpha, int beta);
        void sortMoves(moves *moveList);
        int evaluatePosition();
        void startSearch();
        int best_move() const;
        void set_time_to_think(const int& t);

        //magic
        Bitboard findMagicNumber(Square square, int relevantBits, bool isBishop) const;
        Bitboard generateMagicNumber() const;
        Bitboard getRandomBitboardnumber() const;
        void initMagicNumbers() const;

        // FEN string i/o
        void FromFEN(std::string FEN);

        // zobrist tt
        Bitboard generateZobristHashKey();
        void ZobristHashingTestSuite();
        void initTranspositionTable();
        int probeHash(int depth, int alpha, int beta, int *move);
        void recordHash(int depth, int evaluation, int hashFlag, int move);
                
        //visuals
        void printMove(int move) const;
        void printBitBoard(Bitboard bitboard) const;
        void printMoveList(moves *moveList) const;
        void printMoveScores(moves *moveList, int *scores);
        void visualizeBoard() const;
        void visualizeButterflyBoard() const;

        //perft
        long long perftDriver(int depth);
        void perftTestSuite();

        //TODO fix these
        void sleep(int seconds);
        void test();
        int undoMove(int move);
};

inline Color Position::to_move() const { return toMove; }

inline Bitboard Position::pieces() const { return occupiedBoard[BOTH]; }

template<typename... PieceType>
inline Bitboard Position::pieces(PieceType... piece) const{
    return (pieceBoard[piece] | ...);
}

inline Bitboard Position::pieces(Color c) const { return occupiedBoard[c]; }

template<typename... PieceType>
inline Bitboard Position::pieces(Color c, PieceType... piece) const{
    return pieces(c) & pieces(piece...);
}

inline int Position::castling_rights() const { return castlingRights; }

inline Square Position::ep_square() const { return enPassant; }

inline int Position::best_move() const { return PVTable[0][0]; }

inline void Position::set_time_to_think(const int& t) { timeToThink = t; }

constexpr Bitboard square_bb(Square s) {
    assert(is_ok(s));
    return (1ULL << s);
}

constexpr void addMove(int move, moves *moveList){
    moveList->moves[moveList->count] = move;
    moveList->count++;
}

inline __attribute__((always_inline)) unsigned int popcount(Bitboard bitboard) {
    return __builtin_popcountll(bitboard);
}

inline __attribute__((always_inline)) Square lsb(Bitboard bitboard) {
    assert(bitboard != 0);
    return Square(__builtin_ctzll(bitboard));
}

constexpr void apply_permutation(int *v, int count, int * indices){
    using std::swap; // to permit Koenig lookup
    for (int i = 0; i < count; i++) {
        auto current = i;
        while (i != indices[current]) {
            auto next = indices[current];
            swap(v[current], v[next]);
            indices[current] = current;
            current = next;
        }
        indices[current] = current;
    }
}

} //namespace EVal
#endif
