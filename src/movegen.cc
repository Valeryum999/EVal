#include "movegen.h"

namespace EVal {

namespace MoveGen {

Bitboard pawnAttacks[2][64];
Bitboard knightAttacks[64];
Bitboard bishopMasks[64];
Bitboard bishopAttacks[64][512];
Bitboard rookMasks[64];
Bitboard rookAttacks[64][4096];
Bitboard kingAttacks[64];

Bitboard getBishopAttacks(Square square, Bitboard occupancy) {
    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopRelevantBits[square];
    return bishopAttacks[square][occupancy];
}
    
Bitboard getRookAttacks(Square square, Bitboard occupancy) {
    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookRelevantBits[square];
    return rookAttacks[square][occupancy];
}
    
Bitboard getQueenAttacks(Square square, Bitboard occupancy) {
    return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
}

int generateRandomLegalMove() {
    moves moveList[1];
    moveList->count = 0;
    // getAllPossibleMoves(moveList);
    int move = 0;
    // do{
    //     int randomIndex = rand() % moveList->count;
    //     move = moveList->moves[randomIndex];
    // } while(!makeMove(move));
    return move;
}

int getAllPossibleMoves(const Position& pos, moves *moveList) {
    moveList->count = 0;
    int result = 0;
    Bitboard bitboard, attacks;
    Square fromSquare, toSquare;
    Color toMove = pos.to_move();
    for(Piece piece=WhitePawn; piece<=BlackKing; ++piece){
        bitboard = pos.pieces(piece);
        if(toMove == WHITE){
            if(piece == WhitePawn){
                while(bitboard){
                    fromSquare = lsb(bitboard);
                    toSquare = fromSquare + 8;
                    if(!(toSquare > h8) && !getBit(pos.pieces(),toSquare)){
                        if(fromSquare >= a7 && fromSquare <= h7){
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteQueen,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteRook,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteBishop,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteKnight,0,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,0,0,0,0,0),moveList);
                            if((fromSquare >= a2 && fromSquare <= h2) 
                                && !getBit(pos.pieces(), (toSquare+8)))
                                addMove(encodeMove(fromSquare,(toSquare+8),WhitePawn,0,0,1,0,0),moveList);
                        }
                    }
                    attacks = pawnAttacks[toMove][fromSquare] & pos.pieces(BLACK);
                    while(attacks){
                        toSquare = lsb(attacks);
                        if(fromSquare >= a7 && fromSquare <= h7){
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteQueen,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteRook,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteBishop,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteKnight,1,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,0,1,0,0,0),moveList);
                        }
                        popBit(attacks,toSquare);
                    }
                    if(pos.ep_square() != noSquare){
                        Bitboard enPassantAttacks = pawnAttacks[WHITE][fromSquare] & (square_bb(pos.ep_square()));
                        if(enPassantAttacks){
                            Square toEnpassant = lsb(enPassantAttacks);
                            addMove(encodeMove(fromSquare,toEnpassant,WhitePawn,0,1,0,1,0),moveList);
                        }
                    }

                    popBit(bitboard, fromSquare);
                }
                continue;
            }
            if(piece == WhiteKing){
                if(pos.castling_rights() & 1){
                    if(!getBit(pos.pieces(),f1) 
                    && !getBit(pos.pieces(),g1)){
                        if(!pos.isSquareAttacked(e1, BLACK) 
                        && !pos.isSquareAttacked(f1,BLACK) 
                        && !pos.isSquareAttacked(g1,BLACK)) addMove(encodeMove(e1,g1,WhiteKing,0,0,0,0,1),moveList);
                    }
                }
                if(pos.castling_rights() & 2){
                    if(!getBit(pos.pieces(),b1)
                    && !getBit(pos.pieces(),c1) 
                    && !getBit(pos.pieces(),d1)){
                        if(!pos.isSquareAttacked(e1,BLACK) 
                        && !pos.isSquareAttacked(c1,BLACK) 
                        && !pos.isSquareAttacked(d1,BLACK)) addMove(encodeMove(e1,c1,WhiteKing,0,0,0,0,1),moveList);
                    }
                }
            }
        } else {
            if(piece == BlackPawn){
                while(bitboard){
                    fromSquare = lsb(bitboard);
                    toSquare = fromSquare - 8;
                    if(!(toSquare < a1) && !getBit(pos.pieces(),toSquare)){
                        if(fromSquare >= a2 && fromSquare <= h2){
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackQueen,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackRook,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackBishop,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackKnight,0,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,0,0,0,0,0),moveList);
                            if((fromSquare >= a7 && fromSquare <= h7) 
                                && !getBit(pos.pieces(), (toSquare-8)))
                                addMove(encodeMove(fromSquare,(toSquare-8),BlackPawn,0,0,1,0,0),moveList);
                        }
                    }
                    attacks = pawnAttacks[toMove][fromSquare] & pos.pieces(WHITE);
                    while(attacks){
                        toSquare = lsb(attacks);
                        if(fromSquare >= a2 && fromSquare <= h2){
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackQueen,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackRook,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackBishop,1,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackKnight,1,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,0,1,0,0,0),moveList);
                        }
                        popBit(attacks,toSquare);
                    }
                    if(pos.ep_square() != noSquare){
                        Bitboard enPassantAttacks = pawnAttacks[BLACK][fromSquare] & (square_bb(pos.ep_square()));
                        if(enPassantAttacks){
                            Square toEnpassant = lsb(enPassantAttacks);
                            addMove(encodeMove(fromSquare,toEnpassant,BlackPawn,0,1,0,1,0),moveList);
                        }
                    }
                    popBit(bitboard, fromSquare);
                }
                continue;
            }
            if(piece == BlackKing){
                if(pos.castling_rights() & 4){
                    if(!getBit(pos.pieces(),f8) 
                    && !getBit(pos.pieces(),g8)){
                        if(!pos.isSquareAttacked(e8,WHITE) 
                        && !pos.isSquareAttacked(f8,WHITE) 
                        && !pos.isSquareAttacked(g8,WHITE)) addMove(encodeMove(e8,g8,BlackKing,0,0,0,0,1),moveList);
                    }
                }
                if(pos.castling_rights() & 8){
                    if(!getBit(pos.pieces(),b8)
                    && !getBit(pos.pieces(),c8) 
                    && !getBit(pos.pieces(),d8)){
                        if(!pos.isSquareAttacked(e8,WHITE) 
                        && !pos.isSquareAttacked(c8,WHITE) 
                        && !pos.isSquareAttacked(d8,WHITE)) addMove(encodeMove(e8,c8,BlackKing,0,0,0,0,1),moveList);
                    }
                }
            }
        }
        if((toMove == WHITE) ? piece == WhiteKnight : piece == BlackKnight){
            while(bitboard){
                fromSquare = lsb(bitboard);
                attacks = knightAttacks[fromSquare] & 
                            ((toMove == WHITE) ? ~pos.pieces(WHITE) : ~pos.pieces(BLACK));
                while(attacks){
                    toSquare = lsb(attacks);
                    if(!getBit(((toMove == WHITE) ? pos.pieces(BLACK) : pos.pieces(WHITE)),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == WHITE) ? piece == WhiteBishop : piece == BlackBishop){
            while(bitboard){
                fromSquare = lsb(bitboard);
                attacks = getBishopAttacks(fromSquare, pos.pieces()) & 
                            ((toMove == WHITE) ? ~pos.pieces(WHITE) : ~pos.pieces(BLACK));
                while(attacks){
                    toSquare = lsb(attacks);
                    if(!getBit(((toMove == WHITE) ? pos.pieces(BLACK) : pos.pieces(WHITE)),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == WHITE) ? piece == WhiteRook : piece == BlackRook){
            while(bitboard){
                fromSquare = lsb(bitboard);
                attacks = getRookAttacks(fromSquare, pos.pieces()) & 
                            ((toMove == WHITE) ? ~pos.pieces(WHITE) : ~pos.pieces(BLACK));
                while(attacks){
                    toSquare = lsb(attacks);
                    if(!getBit(((toMove == WHITE) ? pos.pieces(BLACK) : pos.pieces(WHITE)),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == WHITE) ? piece == WhiteQueen : piece == BlackQueen){
            while(bitboard){
                fromSquare = lsb(bitboard);
                attacks = getQueenAttacks(fromSquare, pos.pieces()) & 
                            ((toMove == WHITE) ? ~pos.pieces(WHITE) : ~pos.pieces(BLACK));
                while(attacks){
                    toSquare = lsb(attacks);
                    if(!getBit(((toMove == WHITE) ? pos.pieces(BLACK) : pos.pieces(WHITE)),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }

        if((toMove == WHITE) ? piece == WhiteKing : piece == BlackKing){
            while(bitboard){
                fromSquare = lsb(bitboard);
                attacks = kingAttacks[fromSquare] & 
                            ((toMove == WHITE) ? ~pos.pieces(WHITE) : ~pos.pieces(BLACK));
                while(attacks){
                    toSquare = lsb(attacks);
                    if(!getBit(((toMove == WHITE) ? pos.pieces(BLACK) : pos.pieces(WHITE)),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
    }
    return result;
}

Bitboard maskKingAttacks(Square square){
    Bitboard attacks = 0;
    Bitboard king = square_bb(square);

    attacks |= king << 7 & notFirstRankHFile;
    attacks |= king << 8 & notFirstRank;  
    attacks |= king << 9 & notFirstRankAFile; 
    attacks |= king << 1 & notAFile; 
    attacks |= king >> 7 & notEighthRankAFile;
    attacks |= king >> 8 & notEighthRank; 
    attacks |= king >> 9 & notEighthRankHFile; 
    attacks |= king >> 1 & notHFile;

    return attacks;
}

Bitboard maskKnightAttacks(Square square){
    Bitboard attacks = 0;
    Bitboard knight = square_bb(square);

    attacks |= knight << 17 & notAFile;
    attacks |= knight << 10 & notABFiles;  
    attacks |= knight >>  6 & notABFiles; 
    attacks |= knight >> 15 & notAFile; 
    attacks |= knight << 15 & notHFile;
    attacks |= knight <<  6 & notGHFiles; 
    attacks |= knight >> 10 & notGHFiles; 
    attacks |= knight >> 17 & notHFile;

    return attacks;
}

Bitboard maskPawnAttacks(Square square, Color color){
    Bitboard attacks = 0;
    Bitboard pawn = square_bb(square);

    if(color == WHITE){
        attacks |= pawn << 7 & notHFile;
        attacks |= pawn << 9 & notAFile;
    } else {
        attacks |= pawn >> 7 & notAFile;
        attacks |= pawn >> 9 & notHFile;
    }

    return attacks;
}

Bitboard maskBishopOccupancy(Square square) {
    Bitboard occupancy = 0;
    int bishopRank = square / 8;
    int bishopFile = square % 8;
    int rank,file;
    for(rank=bishopRank+1, file=bishopFile+1; rank<7 && file<7; rank++, file++)
        occupancy |= square_bb(Square(rank*8 + file));
    for(rank=bishopRank-1, file=bishopFile+1; rank>0 && file<7; rank--, file++)
        occupancy |= square_bb(Square(rank*8 + file));
    for(rank=bishopRank+1, file=bishopFile-1; rank<7 && file>0; rank++, file--)
        occupancy |= square_bb(Square(rank*8 + file));
    for(rank=bishopRank-1, file=bishopFile-1; rank>0 && file>0; rank--, file--)
        occupancy |= square_bb(Square(rank*8 + file));

    return occupancy;
}

Bitboard maskRookOccupancy(Square square) {
    Bitboard occupancy = 0;
    int rookRank = square / 8;
    int rookFile = square % 8;
    int rank,file;
    for(rank=rookRank+1; rank<7; rank++)
        occupancy |= square_bb(Square(rank*8 + rookFile));
    for(rank=rookRank-1; rank>0; rank--)
        occupancy |= square_bb(Square(rank*8 + rookFile));
    for(file=rookFile+1; file<7; file++)
        occupancy |= square_bb(Square(rookRank*8 + file));
    for(file=rookFile-1; file>0; file--)
        occupancy |= square_bb(Square(rookRank*8 + file));


    return occupancy;
}

Bitboard generateBishopAttacksOnTheFly(Square square, const Bitboard& block) {
    Bitboard attacks = 0;
    int bishopRank = square / 8;
    int bishopFile = square % 8;
    int rank = 0;
    int file = 0;
    for(rank=bishopRank+1, file=bishopFile+1; rank<=7 && file<=7; rank++, file++){
        attacks |= square_bb(Square(rank*8 + file));
        if(square_bb(Square(rank*8 + file)) & block) break;
    }
    for(rank=bishopRank-1, file=bishopFile+1; rank>=0 && file<=7; rank--, file++){
        attacks |= square_bb(Square(rank*8 + file));
        if(square_bb(Square(rank*8 + file)) & block) break;
    }
    for(rank=bishopRank+1, file=bishopFile-1; rank<=7 && file>=0; rank++, file--){
        attacks |= square_bb(Square(rank*8 + file));
        if(square_bb(Square(rank*8 + file)) & block) break;
    }
    for(rank=bishopRank-1, file=bishopFile-1; rank>=0 && file>=0; rank--, file--){
        attacks |= square_bb(Square(rank*8 + file));
        if(square_bb(Square(rank*8 + file)) & block) break;
    }

    return attacks;
}

Bitboard generateRookAttacksOnTheFly(Square square, const Bitboard& block) {
    Bitboard attacks = 0;
    int rookRank = square / 8;
    int rookFile = square % 8;
    int rank = 0;
    int file = 0;
    for(rank=rookRank+1; rank<=7; rank++){
        Bitboard attack = square_bb(Square(rank*8 + rookFile));
        attacks |= attack;
        if(attack & block) break;
    }
    for(rank=rookRank-1; rank>=0; rank--){
        Bitboard attack = square_bb(Square(rank*8 + rookFile));
        attacks |= attack;
        if(attack & block) break;
    }
    for(file=rookFile+1; file<=7; file++){
        Bitboard attack = square_bb(Square(rookRank*8 + file));
        attacks |= attack;
        if(attack & block) break;
    }
    for(file=rookFile-1; file>=0; file--){
        Bitboard attack = square_bb(Square(rookRank*8 + file));
        attacks |= attack;
        if(attack & block) break;
    }

    return attacks;
}

Bitboard setOccupancyBits(int index, int bitsInMask, Bitboard occupancy_mask) {
    Bitboard occupancy = 0;

    for(int count=0; count<bitsInMask; count++){
        Square square = lsb(occupancy_mask);
        popBit(occupancy_mask,square);
        if(index & (1 << count))
            occupancy |= square_bb(square);
    }

    return occupancy;
}

void initLeaperPiecesMoves() {
    for(Square square=a1; square<=h8; ++square){
        pawnAttacks[WHITE][square] = maskPawnAttacks(square,WHITE);
        pawnAttacks[BLACK][square] = maskPawnAttacks(square,BLACK);
        knightAttacks[square] = maskKnightAttacks(square);
        kingAttacks[square] = maskKingAttacks(square);
    }
}

void initSliderPiecesMoves(bool bishop) {
    for(Square square=a1; square <= h8; ++square){
        bishopMasks[square] = maskBishopOccupancy(square);
        rookMasks[square] = maskRookOccupancy(square);
        Bitboard occupancyMask = bishop ? bishopMasks[square] : rookMasks[square];
        int relevantpopcount = bishop ? bishopRelevantBits[square] : rookRelevantBits[square];
        int occupancyIndices = 1 << relevantpopcount;

        for(int index=0; index < occupancyIndices; index++){
            if(bishop){
                Bitboard occupancy = setOccupancyBits(index, bishopRelevantBits[square], occupancyMask);
                int magicIndex = (occupancy * bishopMagicNumbers[square]) >> (64-relevantpopcount);
                bishopAttacks[square][magicIndex] = generateBishopAttacksOnTheFly(square,occupancy);
            } else {
                Bitboard occupancy = setOccupancyBits(index, rookRelevantBits[square], occupancyMask);
                int magicIndex = (occupancy * rookMagicNumbers[square]) >> (64-relevantpopcount);
                rookAttacks[square][magicIndex] = generateRookAttacksOnTheFly(square,occupancy);
            }
        }
    }
}

}

} // namespace EVal