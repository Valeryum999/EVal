#ifndef MOVE_GEN_H
#define MOVE_GEN_H

#include "position.h"

namespace EVal {

namespace MoveGen {

extern Bitboard pawnAttacks[2][64];
extern Bitboard knightAttacks[64];
extern Bitboard bishopMasks[64];
extern Bitboard bishopAttacks[64][512];
extern Bitboard rookMasks[64];
extern Bitboard rookAttacks[64][4096];
extern Bitboard kingAttacks[64];


void initLeaperPiecesMoves();
void initSliderPiecesMoves(bool bishop);
Bitboard maskKingAttacks(Square square);
Bitboard maskKnightAttacks(Square square);
Bitboard maskPawnAttacks(Square square, Color color);
Bitboard maskBishopOccupancy(Square square);
Bitboard maskRookOccupancy(Square square);
Bitboard setOccupancyBits(int index, int bitsInMask, Bitboard occupancy_mask);
Bitboard generateBishopAttacksOnTheFly(Square square, const Bitboard& block);
Bitboard generateRookAttacksOnTheFly(Square square, const Bitboard& block);
Bitboard getBishopAttacks(Square square, Bitboard occupancy);
Bitboard getRookAttacks(Square square, Bitboard occupancy);
int getAllPossibleMoves(const Position& pos, moves *moveList);

template<PieceType T>
Bitboard attacks_bb(Square square, Bitboard occupancy){
    assert(is_ok(square));

    switch(T){
        case BISHOP:
            return getBishopAttacks(square, occupancy);
        case ROOK:
            return getRookAttacks(square, occupancy);
        case QUEEN:
            return attacks_bb<BISHOP>(square, occupancy) 
                 | attacks_bb<ROOK>(square, occupancy);
    }
}

} // namespace MoveGen

} // namespace EVal

#endif