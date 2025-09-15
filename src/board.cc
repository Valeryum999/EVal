#include "board.h"

Board::Board() {
    pieceBoard[WhitePawn]     = 0x0000000000000000;
    pieceBoard[WhiteKnight]   = 0x0000000000000000;
    pieceBoard[WhiteBishop]   = 0x0000000000000000;
    pieceBoard[WhiteRook]     = 0x0000000000000000;
    pieceBoard[WhiteQueen]    = 0x0000000000000000;
    pieceBoard[WhiteKing]     = 0x0000000000000000;
    pieceBoard[BlackPawn]     = 0x0000000000000000;
    pieceBoard[BlackKnight]   = 0x0000000000000000;
    pieceBoard[BlackBishop]   = 0x0000000000000000;
    pieceBoard[BlackRook]     = 0x0000000000000000;
    pieceBoard[BlackQueen]    = 0x0000000000000000;
    pieceBoard[BlackKing]     = 0x0000000000000000;
    occupiedBoard[White]      = 0x0000000000000000;
    occupiedBoard[Black]      = 0x0000000000000000;
    occupiedBoard[Both]       = 0x0000000000000000;
    //TODO: implement butterfly board to easily get the piece on a given square
    for(int square=a1; square<=h8; square++) butterflyBoard[square] = NoPiece;
    castlingRights = 0;
    canEnPassant[White] = false;
    canEnPassant[Black] = false;
    enPassant = noSquare;
    toMove = White;
    initLeaperPiecesMoves();
    initSliderPiecesMoves(false);
    initSliderPiecesMoves(true);
    ply = 0;
    maxPly = 0;
    nodes = 0;
    bestMove = 0;
    bestEval = 0;
    searchCancelled = false;
    initTables();
}

void Board::addMove(int move, moves *moveList){
    moveList->moves[moveList->count] = move;
    moveList->count++;
}

void apply_permutation(int *v, int count, int * indices){
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

/**
 * bitScanForward
 * @author Kim Walisch (2012)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
unsigned int Board::bitScanForward(U64 bb) const{
   const U64 debruijn64 = C64(0x03f79d71b4cb0a89);
   //assert (bb != 0);
   return index64[((bb ^ (bb-1)) * debruijn64) >> 58];
}

int Board::evaluatePosition(){
    int middlegame[2];
    int endgame[2];
    int mobility[2];
    int gamePhase = 0;

    middlegame[White] = 0;
    endgame[White] = 0;
    middlegame[Black] = 0;
    endgame[Black] = 0;
    mobility[White] = 0;
    mobility[Black] = 0;

    for(int square=0; square < 64; square++){
        int piece = butterflyBoard[square];
        if(piece == NoPiece) continue;
        middlegame[(piece - 6) < 0 ? White : Black] += mg_table[piece][squareToVisualBoardSquare[square]];
        endgame[(piece - 6) < 0 ? White : Black] += eg_table[piece][squareToVisualBoardSquare[square]];
        gamePhase += gamephaseInc[piece];
        switch(piece){
            case WhitePawn:
            case BlackPawn:
                break;
            case WhiteKnight:
            case BlackKnight:
                if(piece == WhiteKnight)
                    mobility[White] += bitCount(knightAttacks[square] & ~occupiedBoard[White]);
                else
                    mobility[Black] += bitCount(knightAttacks[square] & ~occupiedBoard[Black]);
                break;
            case WhiteBishop:
            case BlackBishop:
                if(piece == WhiteBishop)
                    mobility[White] += bitCount(getBishopAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += bitCount(getBishopAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteRook:
            case BlackRook:
                if(piece == WhiteRook)
                    mobility[White] += bitCount(getRookAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += bitCount(getRookAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteQueen:
            case BlackQueen:
                if(piece == WhiteQueen)
                    mobility[White] += bitCount(getQueenAttacks(square,occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += bitCount(getQueenAttacks(square,occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteKing:
            case BlackKing:
                if(piece == WhiteKing)
                    mobility[White] -= bitCount(kingAttacks[square] & ~occupiedBoard[White]);
                else
                    mobility[Black] -= bitCount(kingAttacks[square] & ~occupiedBoard[Black]);
                break;
        }
    }
    int middlegameScore = middlegame[toMove] - middlegame[1-toMove];
    int endgameScore = endgame[toMove] - endgame[1-toMove];
    int mobilityScore = mobility[toMove] - mobility[1-toMove];
    int middlegamePhase = gamePhase;
    if (middlegamePhase > 24) middlegamePhase = 24; /* in case of early promotion */
    int endgamePhase = 24 - middlegamePhase;
    return (middlegameScore * middlegamePhase + endgameScore * endgamePhase) / 24 + mobilityScore;
}

U64 Board::findMagicNumber(unsigned int square, int relevantBits, bool isBishop) const{
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 usedAttacks[4096];

    U64 occupancyMask = isBishop ? maskBishopOccupancy(square) : maskRookOccupancy(square);

    int occupancyIndices = 1 << relevantBits;

    for(int index=0; index<occupancyIndices; index++){
        occupancies[index] = setOccupancyBits(index, relevantBits, occupancyMask);
        attacks[index] = isBishop ? generateBishopAttacksOnTheFly(square,occupancies[index]) 
                                  : generateRookAttacksOnTheFly(square,occupancies[index]);
    }

    for(int randomCount=0; randomCount < 1000000000; randomCount++){
        U64 magicNumber = generateMagicNumber();
        if(bitCount((occupancyMask * magicNumber) & 0xff00000000000000) < 6) continue;
        memset(usedAttacks,0,sizeof(usedAttacks));
        int index, hasFailed;
        for(index=0, hasFailed=0;!hasFailed && index < occupancyIndices; index++){
            int magicIndex = (occupancies[index] * magicNumber) >> (64-relevantBits);
            if(usedAttacks[magicIndex] == 0){
                usedAttacks[magicIndex] = attacks[index];
            } else if(usedAttacks[magicIndex] != attacks[index]){
                hasFailed = 1;
            }
        }
        if(!hasFailed) return magicNumber;
    }

    //should never happen
    std::cout << "ERROR: Magic Number not found!" << std::endl;
    return 0;
}

void Board::initLeaperPiecesMoves() {
    for(int square=0; square<64; square++){
        pawnAttacks[White][square] = maskPawnAttacks(square,White);
        pawnAttacks[Black][square] = maskPawnAttacks(square,Black);
        knightAttacks[square] = maskKnightAttacks(square);
        kingAttacks[square] = maskKingAttacks(square);
    }
}

void Board::initSliderPiecesMoves(bool bishop) {
    for(int square=0; square < 64; square++){
        bishopMasks[square] = maskBishopOccupancy(square);
        rookMasks[square] = maskRookOccupancy(square);
        U64 occupancyMask = bishop ? bishopMasks[square] : rookMasks[square];
        int relevantBitCount = bishop ? bishopRelevantBits[square] : rookRelevantBits[square];
        int occupancyIndices = 1 << relevantBitCount;

        for(int index=0; index < occupancyIndices; index++){
            if(bishop){
                U64 occupancy = setOccupancyBits(index, bishopRelevantBits[square], occupancyMask);
                int magicIndex = (occupancy * bishopMagicNumbers[square]) >> (64-relevantBitCount);
                bishopAttacks[square][magicIndex] = generateBishopAttacksOnTheFly(square,occupancy);
            } else {
                U64 occupancy = setOccupancyBits(index, rookRelevantBits[square], occupancyMask);
                int magicIndex = (occupancy * rookMagicNumbers[square]) >> (64-relevantBitCount);
                rookAttacks[square][magicIndex] = generateRookAttacksOnTheFly(square,occupancy);
            }
        }
    }
}

int Board::generateRandomLegalMove() {
    moves moveList[1];
    moveList->count = 0;
    getAllPossibleMoves(moveList);
    int move = 0;
    do{
        int randomIndex = rand() % moveList->count;
        move = moveList->moves[randomIndex];
    } while(!makeMove(move));
    return move;
}

U64 Board::getBishopAttacks(unsigned int square, U64 occupancy) const{
    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopRelevantBits[square];
    return bishopAttacks[square][occupancy];
}

U64 Board::getRookAttacks(unsigned int square, U64 occupancy) const{
    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookRelevantBits[square];
    return rookAttacks[square][occupancy];
}

U64 Board::getQueenAttacks(unsigned int square, U64 occupancy) const{
    U64 bishopOccupancy = occupancy;
    U64 rookOccupancy = occupancy;
    bishopOccupancy &= bishopMasks[square];
    bishopOccupancy *= bishopMagicNumbers[square];
    bishopOccupancy >>= 64 - bishopRelevantBits[square];
    rookOccupancy &= rookMasks[square];
    rookOccupancy *= rookMagicNumbers[square];
    rookOccupancy >>= 64 - rookRelevantBits[square];
    return bishopAttacks[square][bishopOccupancy] | rookAttacks[square][rookOccupancy];
}

int Board::getAllPossibleMoves(moves *moveList) {
    moveList->count = 0;
    int result = 0;
    U64 bitboard, attacks;
    unsigned int fromSquare, toSquare;
    for(unsigned int piece=WhitePawn; piece<=BlackKing; piece++){
        bitboard = pieceBoard[piece];
        if(toMove == White){
            if(piece == WhitePawn){
                while(bitboard){
                    fromSquare = bitScanForward(bitboard);
                    toSquare = fromSquare + 8;
                    if(!(toSquare > h8) && !getBit(occupiedBoard[Both],toSquare)){
                        if(fromSquare >= a7 && fromSquare <= h7){
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteQueen,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteRook,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteBishop,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,WhiteKnight,0,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,WhitePawn,0,0,0,0,0),moveList);
                            if((fromSquare >= a2 && fromSquare <= h2) 
                                && !getBit(occupiedBoard[Both], (toSquare+8)))
                                addMove(encodeMove(fromSquare,(toSquare+8),WhitePawn,0,0,1,0,0),moveList);
                        }
                    }
                    attacks = pawnAttacks[toMove][fromSquare] & occupiedBoard[Black];
                    while(attacks){
                        toSquare = bitScanForward(attacks);
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
                    if(enPassant != noSquare){
                        U64 enPassantAttacks = pawnAttacks[White][fromSquare] & (C64(1) << enPassant);
                        if(enPassantAttacks){
                            unsigned int toEnpassant = bitScanForward(enPassantAttacks);
                            addMove(encodeMove(fromSquare,toEnpassant,WhitePawn,0,1,0,1,0),moveList);
                        }
                    }

                    popBit(bitboard, fromSquare);
                }
                continue;
            }
            if(piece == WhiteKing){
                if(castlingRights & 1){
                    if(!getBit(occupiedBoard[Both],f1) && !getBit(occupiedBoard[Both],g1)){
                        if(!isSquareAttacked(e1, Black) 
                        && !isSquareAttacked(f1,Black) 
                        && !isSquareAttacked(g1,Black)) addMove(encodeMove(e1,g1,WhiteKing,0,0,0,0,1),moveList);
                    }
                }
                if(castlingRights & 2){
                    if(!getBit(occupiedBoard[Both],b1)
                    && !getBit(occupiedBoard[Both],c1) 
                    && !getBit(occupiedBoard[Both],d1)){
                        if(!isSquareAttacked(e1,Black) 
                        && !isSquareAttacked(c1,Black) 
                        && !isSquareAttacked(d1,Black)) addMove(encodeMove(e1,c1,WhiteKing,0,0,0,0,1),moveList);
                    }
                }
            }
        } else {
            if(piece == BlackPawn){
                while(bitboard){
                    fromSquare = bitScanForward(bitboard);
                    toSquare = fromSquare - 8;
                    if(!(toSquare < a1) && !getBit(occupiedBoard[Both],toSquare)){
                        if(fromSquare >= a2 && fromSquare <= h2){
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackQueen,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackRook,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackBishop,0,0,0,0),moveList);
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,BlackKnight,0,0,0,0),moveList);
                        } else {
                            addMove(encodeMove(fromSquare,toSquare,BlackPawn,0,0,0,0,0),moveList);
                            if((fromSquare >= a7 && fromSquare <= h7) 
                                && !getBit(occupiedBoard[Both], (toSquare-8)))
                                addMove(encodeMove(fromSquare,(toSquare-8),BlackPawn,0,0,1,0,0),moveList);
                        }
                    }
                    attacks = pawnAttacks[toMove][fromSquare] & occupiedBoard[White];
                    while(attacks){
                        toSquare = bitScanForward(attacks);
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
                    if(enPassant != noSquare){
                        U64 enPassantAttacks = pawnAttacks[Black][fromSquare] & (C64(1) << enPassant);
                        if(enPassantAttacks){
                            unsigned int toEnpassant = bitScanForward(enPassantAttacks);
                            addMove(encodeMove(fromSquare,toEnpassant,BlackPawn,0,1,0,1,0),moveList);
                        }
                    }
                    popBit(bitboard, fromSquare);
                }
                continue;
            }
            if(piece == BlackKing){
                if(castlingRights & 4){
                    if(!getBit(occupiedBoard[Both],f8) 
                    && !getBit(occupiedBoard[Both],g8)){
                        if(!isSquareAttacked(e8,White) 
                        && !isSquareAttacked(f8,White) 
                        && !isSquareAttacked(g8,White)) addMove(encodeMove(e8,g8,BlackKing,0,0,0,0,1),moveList);
                    }
                }
                if(castlingRights & 8){
                    if(!getBit(occupiedBoard[Both],b8)
                    && !getBit(occupiedBoard[Both],c8) 
                    && !getBit(occupiedBoard[Both],d8)){
                        if(!isSquareAttacked(e8,White) 
                        && !isSquareAttacked(c8,White) 
                        && !isSquareAttacked(d8,White)) addMove(encodeMove(e8,c8,BlackKing,0,0,0,0,1),moveList);
                    }
                }
            }
        }
        if((toMove == White) ? piece == WhiteKnight : piece == BlackKnight){
            while(bitboard){
                fromSquare = bitScanForward(bitboard);
                attacks = knightAttacks[fromSquare] & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = bitScanForward(attacks);
                    if(!getBit(((toMove == White) ? occupiedBoard[Black] : occupiedBoard[White]),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == White) ? piece == WhiteBishop : piece == BlackBishop){
            while(bitboard){
                fromSquare = bitScanForward(bitboard);
                attacks = getBishopAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = bitScanForward(attacks);
                    if(!getBit(((toMove == White) ? occupiedBoard[Black] : occupiedBoard[White]),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == White) ? piece == WhiteRook : piece == BlackRook){
            while(bitboard){
                fromSquare = bitScanForward(bitboard);
                attacks = getRookAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = bitScanForward(attacks);
                    if(!getBit(((toMove == White) ? occupiedBoard[Black] : occupiedBoard[White]),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }
        if((toMove == White) ? piece == WhiteQueen : piece == BlackQueen){
            while(bitboard){
                fromSquare = bitScanForward(bitboard);
                attacks = getQueenAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = bitScanForward(attacks);
                    if(!getBit(((toMove == White) ? occupiedBoard[Black] : occupiedBoard[White]),toSquare)){
                        addMove(encodeMove(fromSquare,toSquare,piece,0,0,0,0,0),moveList);
                    } else {
                        addMove(encodeMove(fromSquare,toSquare,piece,0,1,0,0,0),moveList);
                    }
                    popBit(attacks,toSquare);
                }
                popBit(bitboard, fromSquare);
            }
        }

        if((toMove == White) ? piece == WhiteKing : piece == BlackKing){
            while(bitboard){
                fromSquare = bitScanForward(bitboard);
                attacks = kingAttacks[fromSquare] & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = bitScanForward(attacks);
                    if(!getBit(((toMove == White) ? occupiedBoard[Black] : occupiedBoard[White]),toSquare)){
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

U64 Board::getRandomU64number() const{
    U64 n1,n2,n3,n4;
    n1 = (U64)(random() & 0xffff);
    n2 = (U64)(random() & 0xffff);
    n3 = (U64)(random() & 0xffff);
    n4 = (U64)(random() & 0xffff);

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

U64 Board::generateMagicNumber() const{
    return getRandomU64number() & getRandomU64number() & getRandomU64number(); 
}

// Checks if given square is attacked by given color
bool Board::isSquareAttacked(unsigned int square,ColorType color) const{
    if((color == White) && (pawnAttacks[Black][square] & pieceBoard[WhitePawn])) return true;
    if((color == Black) && (pawnAttacks[White][square] & pieceBoard[BlackPawn])) return true;
    if(knightAttacks[square] & ((color == White) ? pieceBoard[WhiteKnight] : pieceBoard[BlackKnight])) return true;
    if(getBishopAttacks(square,occupiedBoard[Both]) & ((color == White) ? pieceBoard[WhiteBishop] : pieceBoard[BlackBishop])) return true;
    if(getRookAttacks(square,occupiedBoard[Both]) & ((color == White) ? pieceBoard[WhiteRook] : pieceBoard[BlackRook])) return true;
    if(getQueenAttacks(square,occupiedBoard[Both]) & ((color == White) ? pieceBoard[WhiteQueen] : pieceBoard[BlackQueen])) return true;
    if(kingAttacks[square] & ((color == White) ? pieceBoard[WhiteKing] : pieceBoard[BlackKing])) return true;
    return false;
}

void Board::initMagicNumbers() const{
    for(int square=0; square<64; square++){
        printf(" 0x%llxULL,\n", findMagicNumber(square,bishopRelevantBits[square], true));
    }

    for(int square=0; square<64; square++){
        printf(" 0x%llxULL,\n", findMagicNumber(square,rookRelevantBits[square], false));
    }
}

void Board::initTables() {
    for (int piece = WhitePawn; piece <= WhiteKing; piece++) {
        for (int square = 0; square < 64; square++) {
            mg_table[piece][square] = mg_value[piece] + mg_pesto_table[piece][square];
            eg_table[piece][square] = eg_value[piece] + eg_pesto_table[piece][square];
            mg_table[piece+6][square] = mg_value[piece] + mg_pesto_table[piece][square^56];
            eg_table[piece+6][square] = eg_value[piece] + eg_pesto_table[piece][square^56];
        }
    }
}

int Board::makeMove(int move){
    copyBoard();
    unsigned int fromSquare = getFrom(move);
    unsigned int toSquare = getTo(move);
    U64 bbFromSquare = C64(1) << fromSquare;
    U64 bbToSquare = C64(1) << toSquare;
    U64 bbFromToSquare = bbFromSquare ^ bbToSquare;
    unsigned int piece = getPiece(move);
    unsigned int promoted = getPromotedPiece(move);
    pieceBoard[piece] ^= bbFromToSquare;
    occupiedBoard[toMove] ^= bbFromToSquare;
    occupiedBoard[Both] ^= bbFromToSquare;
    if(getCaptureFlag(move)){
        if(butterflyBoard[toSquare] != NoPiece){
            pieceBoard[butterflyBoard[toSquare]] ^= bbToSquare;
            occupiedBoard[1-toMove] ^= bbToSquare;
            occupiedBoard[Both] ^= bbToSquare;
        }
    }
    butterflyBoard[fromSquare] = NoPiece;
    butterflyBoard[toSquare] = piece;
    if(promoted){
        pieceBoard[piece] ^= bbToSquare;
        pieceBoard[promoted] ^= bbToSquare;
        butterflyBoard[toSquare] = promoted;
    }
    if(getEnPassantFlag(move)){
        if(toMove == White){
            popBit(pieceBoard[BlackPawn],(toSquare-8));
            popBit(occupiedBoard[Black],(toSquare-8));
            popBit(occupiedBoard[Both],(toSquare-8));
            butterflyBoard[toSquare-8] = NoPiece;
        } else {
            popBit(pieceBoard[WhitePawn],(toSquare+8));
            popBit(occupiedBoard[White],(toSquare+8));
            popBit(occupiedBoard[Both],(toSquare+8));
            butterflyBoard[toSquare+8] = NoPiece;
        }         
    }
    enPassant = noSquare;
    if(getDoublePawnPushFlag(move)){
        enPassant = (Square)((toMove == White) ? toSquare - 8 : toSquare + 8);
    }
    if(getCastlingFlag(move)){
        switch(toSquare){
            case g1:
                pieceBoard[WhiteRook] ^= h1f1;
                occupiedBoard[White] ^= h1f1;
                occupiedBoard[Both] ^= h1f1;
                butterflyBoard[h1] = NoPiece;
                butterflyBoard[f1] = WhiteRook;
                break;
            case c1:
                pieceBoard[WhiteRook] ^= a1d1;
                occupiedBoard[White]  ^= a1d1;
                occupiedBoard[Both]   ^= a1d1;
                butterflyBoard[a1] = NoPiece;
                butterflyBoard[d1] = WhiteRook;
                break;
            case g8:
                pieceBoard[BlackRook] ^= h8f8;
                occupiedBoard[Black]  ^= h8f8;
                occupiedBoard[Both]   ^= h8f8;
                butterflyBoard[h8] = NoPiece;
                butterflyBoard[f8] = BlackRook;
                break;
            case c8:
                pieceBoard[BlackRook] ^= a8d8;
                occupiedBoard[Black]  ^= a8d8;
                occupiedBoard[Both]   ^= a8d8;
                butterflyBoard[a8] = NoPiece;
                butterflyBoard[d8] = BlackRook;
                break;
        }     
    }
    castlingRights &= castlingRightsTable[fromSquare];
    castlingRights &= castlingRightsTable[toSquare];

    toMove = (ColorType)(1-toMove);
    if(isSquareAttacked((toMove == White) ? bitScanForward(pieceBoard[BlackKing]) : bitScanForward(pieceBoard[WhiteKing]),toMove)){
            restoreBoard();
            return 0;
        }
    return 1; 
}

void Board::test() {
    moves moveList[1];
    getAllPossibleMoves(moveList);
    printMoveList(moveList);

    for(int i=0; i<moveList->count; i++){
        int move = moveList->moves[i];
        copyBoard();
        if(!makeMove(move)){
            std::cout << "Skipped move: ";
            printMove(move);
            continue;
        }
        visualizeBoard();
        getchar();
        restoreBoard();
        visualizeBoard();
        getchar();
    }
}

long long Board::perftDriver(int depth){
    if(depth == 0){
        return 1;
    }
    long long nodes = 0;
    moves moveList[1];
    getAllPossibleMoves(moveList);
    
    for(int i=0; i<moveList->count; i++){
        copyBoard();
        if(!makeMove(moveList->moves[i])) continue;
        nodes += perftDriver(depth-1);
        restoreBoard();
    }
    return nodes;
}

void Board::perftTestSuite(){
    FromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // starting position
    long long knownNodesStartingPosition[7] = {
        20,400,8902,197281,4865609,119060324,0
    };
    std::string correct = u8"✅";
    std::string incorrect = u8"❌";
    auto start_time = std::chrono::high_resolution_clock::now();
    for(int i=1;i<7;i++){
        long long nodes = perftDriver(i);
        std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions" << (i <= 3 ? "\t\t" : "\t") 
                  << ((nodes == knownNodesStartingPosition[i-1]) ? correct : incorrect) << std::endl;
        if(nodes != knownNodesStartingPosition[i-1]) printf("Wtf nodes %lld\n", knownNodesStartingPosition[i-1]);
    }
    FromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    knownNodesStartingPosition[0] = 48;
    knownNodesStartingPosition[1] = 2039;
    knownNodesStartingPosition[2] = 97862;
    knownNodesStartingPosition[3] = 4085603;
    knownNodesStartingPosition[4] = 193690690;
    for(int i=1;i<6;i++){
        long long nodes = perftDriver(i);
        std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions" << (i <= 2 ? "\t\t" : "\t") 
                  << ((nodes == knownNodesStartingPosition[i-1]) ? correct : incorrect) << std::endl;
        if(nodes != knownNodesStartingPosition[i-1]) printf("Wtf nodes %lld\n", knownNodesStartingPosition[i-1]);
    }
    FromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    knownNodesStartingPosition[0] = 14;
    knownNodesStartingPosition[1] = 191;
    knownNodesStartingPosition[2] = 2812;
    knownNodesStartingPosition[3] = 43238;
    knownNodesStartingPosition[4] = 674624;
    knownNodesStartingPosition[5] = 11030083;
    knownNodesStartingPosition[6] = 178633661;
    for(int i=1;i<8;i++){
        long long nodes = perftDriver(i);
        std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions" << (i <= 3 ? "\t\t" : "\t") 
                  << ((nodes == knownNodesStartingPosition[i-1]) ? correct : incorrect) << std::endl;
        if(nodes != knownNodesStartingPosition[i-1]) printf("Wtf nodes %lld\n", knownNodesStartingPosition[i-1]);
    }
    FromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    knownNodesStartingPosition[0] = 6;
    knownNodesStartingPosition[1] = 264;
    knownNodesStartingPosition[2] = 9467;
    knownNodesStartingPosition[3] = 422333;
    knownNodesStartingPosition[4] = 15833292;
    knownNodesStartingPosition[5] = 706045033;
    for(int i=1;i<6;i++){
        long long nodes = perftDriver(i);
        std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions" << (i <= 3 ? "\t\t" : "\t") 
                  << ((nodes == knownNodesStartingPosition[i-1]) ? correct : incorrect) << std::endl;
        if(nodes != knownNodesStartingPosition[i-1]) printf("Wtf nodes %lld\n", knownNodesStartingPosition[i-1]);
    }
    FromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    knownNodesStartingPosition[0] = 44;
    knownNodesStartingPosition[1] = 1486;
    knownNodesStartingPosition[2] = 62379;
    knownNodesStartingPosition[3] = 2103487;
    knownNodesStartingPosition[4] = 89941194;
    for(int i=1;i<5;i++){
        long long nodes = perftDriver(i);
        std::cout << "[+] Depth: " << i << " ply Result: " << nodes << " positions" << (i <= 2 ? "\t\t" : "\t") 
        << ((nodes == knownNodesStartingPosition[i-1]) ? correct : incorrect) << std::endl;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Test suite completed in " << duration.count() << " milliseconds" << std::endl;
}

U64 Board::maskKingAttacks(unsigned int square){
    U64 attacks = 0;
    U64 king = C64(1) << square;

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

U64 Board::maskKnightAttacks(unsigned int square){
    U64 attacks = 0;
    U64 knight = C64(1) << square;

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

U64 Board::maskPawnAttacks(unsigned int square, ColorType color){
    U64 attacks = 0;
    U64 pawn = C64(1) << square;

    if(color == White){
        attacks |= pawn << 7 & notHFile;
        attacks |= pawn << 9 & notAFile;
    } else {
        attacks |= pawn >> 7 & notAFile;
        attacks |= pawn >> 9 & notHFile;
    }

    return attacks;
}

U64 Board::maskBishopOccupancy(unsigned int square) const{
    U64 occupancy = 0;
    int bishopRank = square / 8;
    int bishopFile = square % 8;
    int rank,file;
    for(rank=bishopRank+1, file=bishopFile+1; rank<7 && file<7; rank++, file++)
        occupancy |= C64(1) << (rank*8 + file);
    for(rank=bishopRank-1, file=bishopFile+1; rank>0 && file<7; rank--, file++)
        occupancy |= C64(1) << (rank*8 + file);
    for(rank=bishopRank+1, file=bishopFile-1; rank<7 && file>0; rank++, file--)
        occupancy |= C64(1) << (rank*8 + file);
    for(rank=bishopRank-1, file=bishopFile-1; rank>0 && file>0; rank--, file--)
        occupancy |= C64(1) << (rank*8 + file);

    return occupancy;
}

U64 Board::maskRookOccupancy(unsigned int square) const{
    U64 occupancy = 0;
    int rookRank = square / 8;
    int rookFile = square % 8;
    int rank,file;
    for(rank=rookRank+1; rank<7; rank++)
        occupancy |= C64(1) << (rank*8 + rookFile);
    for(rank=rookRank-1; rank>0; rank--)
        occupancy |= C64(1) << (rank*8 + rookFile);
    for(file=rookFile+1; file<7; file++)
        occupancy |= C64(1) << (rookRank*8 + file);
    for(file=rookFile-1; file>0; file--)
        occupancy |= C64(1) << (rookRank*8 + file);


    return occupancy;
}

U64 Board::generateBishopAttacksOnTheFly(unsigned int square, U64 block) const{
    U64 attacks = 0;
    int bishopRank = square / 8;
    int bishopFile = square % 8;
    int rank,file;
    for(rank=bishopRank+1, file=bishopFile+1; rank<=7 && file<=7; rank++, file++){
        attacks |= C64(1) << (rank*8 + file);
        if((C64(1) << (rank*8 + file)) & block) break;
    }
    for(rank=bishopRank-1, file=bishopFile+1; rank>=0 && file<=7; rank--, file++){
        attacks |= C64(1) << (rank*8 + file);
        if((C64(1) << (rank*8 + file)) & block) break;
    }
    for(rank=bishopRank+1, file=bishopFile-1; rank<=7 && file>=0; rank++, file--){
        attacks |= C64(1) << (rank*8 + file);
        if((C64(1) << (rank*8 + file)) & block) break;
    }
    for(rank=bishopRank-1, file=bishopFile-1; rank>=0 && file>=0; rank--, file--){
        attacks |= C64(1) << (rank*8 + file);
        if((C64(1) << (rank*8 + file)) & block) break;
    }

    return attacks;
}

U64 Board::generateRookAttacksOnTheFly(unsigned int square, U64 block) const{
    U64 attacks = 0;
    int rookRank = square / 8;
    int rookFile = square % 8;
    int rank,file;
    for(rank=rookRank+1; rank<=7; rank++){
        attacks |= C64(1) << (rank*8 + rookFile);
        if((C64(1) << (rank*8 + rookFile)) & block) break;
    }
    for(rank=rookRank-1; rank>=0; rank--){
        attacks |= C64(1) << (rank*8 + rookFile);
        if((C64(1) << (rank*8 + rookFile)) & block) break;
    }
    for(file=rookFile+1; file<=7; file++){
        attacks |= C64(1) << (rookRank*8 + file);
        if((C64(1) << (rookRank*8 + file)) & block) break;
    }
    for(file=rookFile-1; file>=0; file--){
        attacks |= C64(1) << (rookRank*8 + file);
        if((C64(1) << (rookRank*8 + file)) & block) break;
    }


    return attacks;
}

Square Board::parseSquare(std::string square){
    int file = square[0] - 'a';
    int rank = square[1] - '1';
    return (Square)(rank * 8 + file);
}

int Board::parseMove(std::string uciMove) {
    moves moveList[1];
    getAllPossibleMoves(moveList);
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

void Board::parseGo(std::string go){
    std::vector<std::string> command = split(go," ");
    // if(command[1] == "depth"){
    //     if(command[1] != *command.end())
    //         depth = atoi(command[2].c_str());
    // }
    startSearch();
    printf("\n");
    std::cout << "bestmove ";
    printMoveUCI(PVTable[0][0]);
    std::cout << std::endl;
    //printMoveUCI(generateRandomLegalMove());
}

void Board::parsePosition(std::string position){
    std::vector<std::string> command = split(position," ");
    if(command[1] == "startpos"){
        FromFEN(startingPosition);
        if(command[2] == "moves") {
            for(auto it=command.begin()+3; it!=command.end(); it++){
                makeMove(parseMove(*it));
            }
        }
    } else if(command[1] == "fen"){
        int FENlength = position.find(command[8]) - position.find(command[2]);
        std::string FEN = position.substr(position.find(command[2]),FENlength);
        FromFEN(FEN);
        if(command[8] == "moves"){
            for(auto it=command.begin()+9; it!=command.end(); it++){
                makeMove(parseMove(*it));
            }
        }
    }
    visualizeBoard();
}

void Board::printBitBoard(U64 bitboard) const{
    std::cout << std::endl << std::endl;
    for(int rank=7; rank>=0; rank--){
        std::cout << "  " << rank+1 << "  ";
        for(int file=0; file<8; file++){
            std::cout << getBit(bitboard,(rank*8+file))  << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "     a b c d e f g h" << std::endl;
}

int Board::quiescienceSearch(int alpha, int beta){
    nodes++;
    // PVLength[ply] = ply;
    int currentEval = evaluatePosition();
    if(searchCancelled) return currentEval;
    if(currentEval >= beta) return beta;
    if(currentEval > alpha) alpha = currentEval;
    moves moveList[1];
    getAllPossibleMoves(moveList);
    sortMoves(moveList);
    for(int i=0; i<moveList->count; i++){
        if(!getCaptureFlag(moveList->moves[i])) break; // ordered moves contain only captures for now
        copyBoard();
        if(!makeMove(moveList->moves[i])) continue;
        ply++;
        int evaluation = -quiescienceSearch(-beta, -alpha);
        ply--;
        restoreBoard();
        if(evaluation >= beta){
            //Move was too good, opponent will avoid this position
            return beta; // pruned branch
        }
        if(evaluation > alpha){
            alpha = evaluation;
        }
    }
    return alpha;
}

void Board::printMove(int move) const{
    std::cout << "   "  << squares[getFrom(move)] 
                        << squares[getTo(move)] 
                        << promotedPieces[getPromotedPiece(move)]
                        << "       " << unicodePieces[getPiece(move)]
                        << (getCaptureFlag(move) ? "   capture " : "")
                        << (getDoublePawnPushFlag(move) ? "    double pawn push " : "")
                        << (getEnPassantFlag(move) ? "    en passant " : "")
                        << (getCastlingFlag(move) ? "    castle" : "") << std::endl;
}

void Board::printMoveUCI(int move) const{
    std::cout << squares[getFrom(move)]
              << squares[getTo(move)]
              << ((getPromotedPiece(move)) ? promotedPieces[getPromotedPiece(move)] : '\0');
}

void Board::printMoveList(moves *moveList) const{
    if(!moveList->count) return;
    std::cout << std::endl << "   move      piece    capture    double    enpassant   castling";
    std::cout << std::endl << std::endl;
    for(int i=0; i < moveList->count; i++){
        int move = moveList->moves[i];
        std::cout << "   "  << squares[getFrom(move)] 
                            << squares[getTo(move)] 
                            << promotedPieces[getPromotedPiece(move)]
                            << "       " << unicodePieces[getPiece(move)] << "         "
                            << (getCaptureFlag(move) ? 1 : 0) << "         "
                            << (getDoublePawnPushFlag(move) ? 1 : 0) << "         "
                            << (getEnPassantFlag(move) ? 1 : 0) << "            "
                            << (getCastlingFlag(move) ? 1 : 0) << std::endl;
    }
}

int Board::scoreMove(int move){
    if(getCaptureFlag(move)){
        int targetPiece = butterflyBoard[getTo(move)];
        return MVV_LVA[getPiece(move)][targetPiece] + 10000;
    } else {
        if(killerMoves[0][ply] == move) return 9000;
        else if(killerMoves[1][ply] == move) return 8000;
        else return historyMoves[getPiece(move)][getTo(move)];
    }
    return 0;
}

int Board::searchBestMove(int depth, int alpha, int beta){
    nodes++;
    bool foundPV = false;
    PVLength[ply] = ply;
    if(searchCancelled) return evaluatePosition();
    if(depth == 0) {
        return quiescienceSearch(alpha,beta);
    }
    int legalMoves = 0;
    moves moveList[1];
    moveList->count = 0;
    getAllPossibleMoves(moveList);
    sortMoves(moveList);
    for(int i=0; i<moveList->count; i++){
        copyBoard();
        if(!makeMove(moveList->moves[i])) continue;
        legalMoves++;
        ply++;
        int evaluation;
        if(foundPV){
            evaluation = -searchBestMove(depth-1, -alpha-1, -alpha);
            if(evaluation > alpha && evaluation < beta)
                evaluation = -searchBestMove(depth-1, -beta, -alpha);
        } else 
            evaluation = -searchBestMove(depth-1, -beta, -alpha);
        
        ply--;
        restoreBoard();
        if(evaluation >= beta){
            //Move was too good, opponent will avoid this position
            killerMoves[1][ply] = killerMoves[0][ply];
            killerMoves[0][ply] = moveList->moves[i];
            return beta; // pruned branch
        }
        if(evaluation > alpha){
            foundPV = true;
            if(!getCaptureFlag(moveList->moves[i]))
                historyMoves[getPiece(moveList->moves[i])][getTo(moveList->moves[i])] += depth; //remember this move as it improves our position
            alpha = evaluation;
            PVTable[ply][ply] = moveList->moves[i];
            for(int next = ply+1; next < PVLength[ply+1]; next++)
                PVTable[ply][next] = PVTable[ply+1][next];
            PVLength[ply] = PVLength[ply+1];
        }
        
    }

    if(legalMoves == 0){
        if(isSquareAttacked(bitScanForward(pieceBoard[(toMove == White) ? WhiteKing : BlackKing]),(ColorType)(1-toMove))){
            return -50000+ply;
        }
        return 0;
    }

    return alpha;
}

U64 Board::setOccupancyBits(int index, int bitsInMask, U64 occupancy_mask) const{
    U64 occupancy = 0;

    for(int count=0; count<bitsInMask; count++){
        int square = bitScanForward(occupancy_mask);
        popBit(occupancy_mask,square);
        if(index & (1 << count))
            occupancy |= C64(1) << square;
    }

    return occupancy;
}

void Board::sleep(int seconds){
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    searchCancelled = true;
}

void Board::sortMoves(moves *moveList){
    int scores[moveList->count];
    std::vector<int> indices(moveList->count);
    std::iota(indices.begin(),indices.end(),0);
    for(int i=0; i<moveList->count; i++){
        scores[i] = scoreMove(moveList->moves[i]);
    }
    std::sort(indices.begin(), indices.end(),[&](int a, int b) -> int {
                return scores[a] > scores[b];
            });
    apply_permutation(moveList->moves, moveList->count,&indices[0]);
}

void Board::printMoveScores(moves *moveList, int *scores){
    printf("Debugging sort moves: \n");
    for(int i=0;i<moveList->count;i++){
        printf("move: ");
        printMoveUCI(moveList->moves[i]);
        printf(" score: %d\n", scores[i]);
    }
    printf("\n");
}

std::vector<std::string> Board::split(std::string s,std::string delimiter){
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

void Board::startSearch() {
    searchCancelled = false;
    maxPly = 0;
    std::thread sleeper(&Board::sleep, this, 3);
    sleeper.detach();
    int bestEvalSoFar = 0;
    int PVLengthCopy[64];
    int PVTableCopy[64][64];
    auto start_time = std::chrono::high_resolution_clock::now();
    for(searchDepth = 1; searchDepth < 64; searchDepth++){
        nodes = 0;
        bestEval = searchBestMove(searchDepth, -50000, 50000);
        if(searchCancelled) {
            printf("Stopped search at depth: %d\n", searchDepth);
            break;
        }
        // printf("Current evaluation %.2f\n", bestEval/100*((toMove == White) ? 1 : -1));
        // printf("Max ply reached: %d\n", maxPly);
        printf("info score cp %d depth %d nodes %lld pv", bestEval, searchDepth, nodes);
        for(int i=0; i<PVLength[0]; i++){
            printf(" ");
            printMoveUCI(PVTable[0][i]);
        }
        std::cout << std::endl;
        memcpy(PVLengthCopy, PVLength, 256);
        memcpy(PVTableCopy, PVTable, 16384);
        bestEvalSoFar = bestEval;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Search completed in " << duration.count() << " milliseconds" << std::endl;
    memcpy(PVLength, PVLengthCopy, 256);
    memcpy(PVTable, PVTableCopy, 16384);
    bestEval = bestEvalSoFar;
}

/**
 * swap n none overlapping bits of bit-index i with j
 * @param b any bitboard
 * @param i,j positions of bit sequences to swap
 * @param n number of consecutive bits to swap
 * @return bitboard b with swapped bit-sequences
 */
U64 Board::swapNBits(U64 b, int i, int j, int n) {
   U64     m = ( 1 << n) - 1;
   U64     x = ((b >> i) ^ (b >> j)) & m;
   return  b ^  (x << i) ^ (x << j);
}

void Board::UCImainLoop() {
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
            parsePosition(input);
            continue;
        }
        if(input == "ucinewgame"){
            parsePosition("position startpos");
            continue;
        }
        if(input.find("go") == 0){
            parseGo(input);
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

void Board::visualizeBoard() const{
    std::string renderBoard[8][32];
    for(int rank=0; rank<8; rank++){
        for(int file=0; file<8; file++){
            if(getBit(pieceBoard[WhitePawn],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whitePawn;
                continue;
            }
            if(getBit(pieceBoard[BlackPawn],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackPawn;
                continue;
            }
            if(getBit(pieceBoard[WhiteKnight],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteKnight;
                continue;
            }
            if(getBit(pieceBoard[BlackKnight],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackKnight;
                continue;
            }
            if(getBit(pieceBoard[WhiteBishop],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteBishop;
                continue;
            }
            if(getBit(pieceBoard[BlackBishop],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackBishop;
                continue;
            }
            if(getBit(pieceBoard[WhiteRook],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteRook;
                continue;
            }
            if(getBit(pieceBoard[BlackRook],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackRook;
                continue;
            }
            if(getBit(pieceBoard[WhiteQueen],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteQueen;
                continue;
            }
            if(getBit(pieceBoard[BlackQueen],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackQueen;
                continue;
            }
            if(getBit(pieceBoard[WhiteKing],(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteKing;
                continue;
            }
            if(getBit(pieceBoard[BlackKing],(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackKing;
                continue;
            }
            renderBoard[rank][file] = '.';
        }
    }
    
    for(int rank=7; rank>=0; rank--){
        std::cout << "  " << rank+1 << "  "; 
        for(int file=0; file<8; file++){
            std::cout << renderBoard[rank][file] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "     a b c d e f g h" << std::endl;
    std::cout << std::endl;
    std::cout << "        Color to move: " << colors[toMove] << std::endl;
    std::cout << "        En Passant: " <<  squares[enPassant] << std::endl;
    std::cout << "        Castling: " << ((castlingRights & 1) ? 'K' : '-')
                                        << ((castlingRights & 2) ? 'Q' : '-')
                                        << ((castlingRights & 4) ? 'k' : '-')
                                        << ((castlingRights & 8) ? 'q' : '-') << std::endl;
}

void Board::visualizeButterflyBoard() const{
    std::cout << std::endl;
    for(int rank=7; rank>=0; rank--){
        std::cout << "  " << rank+1 << " ";
        for(int file=0; file<8; file++){
            std::cout << " " << ((butterflyBoard[rank*8+file] != NoPiece) ? unicodePieces[butterflyBoard[rank*8+file]] : ".");
        }
        std::cout << std::endl;
    }
    std::cout << "     a b c d e f g h" << std::endl;
    std::cout << std::endl;
}

void Board::FromFEN(std::string FEN){
    pieceBoard[WhitePawn]     = 0x0000000000000000;
    pieceBoard[WhiteKnight]   = 0x0000000000000000;
    pieceBoard[WhiteBishop]   = 0x0000000000000000;
    pieceBoard[WhiteRook]     = 0x0000000000000000;
    pieceBoard[WhiteQueen]    = 0x0000000000000000;
    pieceBoard[WhiteKing]     = 0x0000000000000000;
    pieceBoard[BlackPawn]     = 0x0000000000000000;
    pieceBoard[BlackKnight]   = 0x0000000000000000;
    pieceBoard[BlackBishop]   = 0x0000000000000000;
    pieceBoard[BlackRook]     = 0x0000000000000000;
    pieceBoard[BlackQueen]    = 0x0000000000000000;
    pieceBoard[BlackKing]     = 0x0000000000000000;
    occupiedBoard[White]      = 0x0000000000000000;
    occupiedBoard[Black]      = 0x0000000000000000;
    occupiedBoard[Both]       = 0x0000000000000000;
    for(int square=a1; square<=h8; square++) butterflyBoard[square] = NoPiece;
    castlingRights = 0;
    unsigned int rank = EIGHT_RANK;
    unsigned int index = 0;
    unsigned int position = 56;
    U64 pawn;
    U64 knight;
    U64 bishop;
    U64 rook;
    U64 queen;
    U64 king;
    while(FEN[index] != ' '){
        switch(FEN[index]){
            case '8':
                emptyBoard |= RANKS[rank];
                position += 8;
                break;
            case '7':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]);
                position += 7;
                break;
            case '6':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]);
                position += 6;
                break;
            case '5':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]
                                           & ~FILES[(position+5)%8]);
                position += 5;
                break;
            case '4':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]
                                           & ~FILES[(position+5)%8]
                                           & ~FILES[(position+4)%8]);
                position += 4;
                break;
            case '3':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]
                                           & ~FILES[(position+5)%8]
                                           & ~FILES[(position+4)%8]
                                           & ~FILES[(position+3)%8]);
                position += 3;
                break;
            
            case '2':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]
                                           & ~FILES[(position+5)%8]
                                           & ~FILES[(position+4)%8]
                                           & ~FILES[(position+3)%8]
                                           & ~FILES[(position+2)%8]);
                position += 2;
                break;
            
            case '1':
                emptyBoard |= (RANKS[rank] & ~FILES[(position+7)%8]
                                           & ~FILES[(position+6)%8]
                                           & ~FILES[(position+5)%8]
                                           & ~FILES[(position+4)%8]
                                           & ~FILES[(position+3)%8]
                                           & ~FILES[(position+2)%8]
                                           & ~FILES[(position+1)%8]);
                position += 1;
                break;

            case 'r':
                rook = C64(1) << position;
                pieceBoard[BlackRook] ^= rook;
                occupiedBoard[Black] ^= rook;
                occupiedBoard[Both] ^= rook;
                butterflyBoard[position] = BlackRook;
                position += 1;
                break;
            case 'R':
                rook = C64(1) << position;
                pieceBoard[WhiteRook] ^= rook;
                occupiedBoard[White] ^= rook;
                occupiedBoard[Both] ^= rook;
                butterflyBoard[position] = WhiteRook;
                position += 1;
                break;
            case 'b':
                bishop = C64(1) << position;
                pieceBoard[BlackBishop] ^= bishop;
                occupiedBoard[Black] ^= bishop;
                occupiedBoard[Both] ^= bishop;
                butterflyBoard[position] = BlackBishop;
                position += 1;
                break;
            case 'B':
                bishop = C64(1) << position;
                pieceBoard[WhiteBishop] ^= bishop;
                occupiedBoard[White] ^= bishop;
                occupiedBoard[Both] ^= bishop;
                butterflyBoard[position] = WhiteBishop;
                position += 1;
                break;
            case 'n':
                knight = C64(1) << position;
                pieceBoard[BlackKnight] ^= knight;
                occupiedBoard[Black] ^= knight;
                occupiedBoard[Both] ^= knight;
                butterflyBoard[position] = BlackKnight;
                position += 1;
                break;
            case 'N':
                knight = C64(1) << position;
                pieceBoard[WhiteKnight] ^= knight;
                occupiedBoard[White] ^= knight;
                occupiedBoard[Both] ^= knight;
                butterflyBoard[position] = WhiteKnight;
                position += 1;
                break;
            case 'q':
                queen = C64(1) << position;
                pieceBoard[BlackQueen] ^= queen;
                occupiedBoard[Black] ^= queen;
                occupiedBoard[Both] ^= queen;
                butterflyBoard[position] = BlackQueen;
                position += 1;
                break;
            case 'Q':
                queen = C64(1) << position;
                pieceBoard[WhiteQueen] ^= queen;
                occupiedBoard[White] ^= queen;
                occupiedBoard[Both] ^= queen;
                butterflyBoard[position] = WhiteQueen;
                position += 1;
                break;
            case 'k':
                king = C64(1) << position;
                pieceBoard[BlackKing] ^= king;
                occupiedBoard[Black] ^= king;
                occupiedBoard[Both] ^= king;
                butterflyBoard[position] = BlackKing;
                position += 1;
                break;
            case 'K':
                king = C64(1) << position;
                pieceBoard[WhiteKing] ^= king;
                occupiedBoard[White] ^= king;
                occupiedBoard[Both] ^= king;
                butterflyBoard[position] = WhiteKing;
                position += 1;
                break;
            case 'p':
                pawn = C64(1) << position;
                pieceBoard[BlackPawn] ^= pawn;
                occupiedBoard[Black] ^= pawn;
                occupiedBoard[Both] ^= pawn;
                butterflyBoard[position] = BlackPawn;
                position += 1;
                break;
            case 'P':
                pawn = C64(1) << position;
                pieceBoard[WhitePawn] ^= pawn;
                occupiedBoard[White] ^= pawn;
                occupiedBoard[Both] ^= pawn;
                butterflyBoard[position] = WhitePawn;
                position += 1;
                break;
            case '/':
                position -= 16;
                rank--;
                break;
            default:
                break;
        }
        index++;
    }
    //2n5/2pP4/1p1p4/2pP3r/KR2Pp1k/8/1p4P1/N7 w - c6 0 1
    index++;
    toMove = (FEN[index] == 'w') ? White : Black;
    index+=2;
    while(FEN[index] != ' '){
        switch(FEN[index]){
            case 'K':
                castlingRights |= 1;
                break;
            case 'Q':
                castlingRights |= 2;
                break;
            case 'k':
                castlingRights |= 4;
                break;
            case 'q':
                castlingRights |= 8;
                break;
            case '-':
                break;
            default:
                break;
        }
        index++;
    }
    index++;
    if(FEN[index] != '-'){
        enPassant = parseSquare(FEN.c_str()+index);
    }
}