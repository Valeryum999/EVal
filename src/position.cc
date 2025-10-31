#include "position.h"

namespace EVal{

transpositionTableType transpositionTable[0x400000];

Position::Position() {
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
    ZobristHashKey = 0;
    didPolyglotFlipEnPassant = false;
    initTranspositionTable();
    timeToThink = 5000; //default 5 seconds
}

constexpr void Position::addMove(int move, moves *moveList){
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

constexpr int Position::evaluatePosition(){
    int middlegame[2] = {0,0};
    int endgame[2] = {0,0};
    int mobility[2] = {0,0};
    int gamePhase = 0;

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
                    mobility[White] += popcount(knightAttacks[square] & ~occupiedBoard[White]);
                else
                    mobility[Black] += popcount(knightAttacks[square] & ~occupiedBoard[Black]);
                break;
            case WhiteBishop:
            case BlackBishop:
                if(piece == WhiteBishop)
                    mobility[White] += popcount(getBishopAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += popcount(getBishopAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteRook:
            case BlackRook:
                if(piece == WhiteRook)
                    mobility[White] += popcount(getRookAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += popcount(getRookAttacks(square, occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteQueen:
            case BlackQueen:
                if(piece == WhiteQueen)
                    mobility[White] += popcount(getQueenAttacks(square,occupiedBoard[Both]) & ~occupiedBoard[White]);
                else
                    mobility[Black] += popcount(getQueenAttacks(square,occupiedBoard[Both]) & ~occupiedBoard[Black]);
                break;
            case WhiteKing:
            case BlackKing:
                if(piece == WhiteKing)
                    mobility[White] -= popcount(kingAttacks[square] & ~occupiedBoard[White]);
                else
                    mobility[Black] -= popcount(kingAttacks[square] & ~occupiedBoard[Black]);
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

Bitboard Position::findMagicNumber(unsigned int square, int relevantBits, bool isBishop) const{
    Bitboard occupancies[4096];
    Bitboard attacks[4096];
    Bitboard usedAttacks[4096];

    Bitboard occupancyMask = isBishop ? maskBishopOccupancy(square) : maskRookOccupancy(square);

    int occupancyIndices = 1 << relevantBits;

    for(int index=0; index<occupancyIndices; index++){
        occupancies[index] = setOccupancyBits(index, relevantBits, occupancyMask);
        attacks[index] = isBishop ? generateBishopAttacksOnTheFly(square,occupancies[index]) 
                                  : generateRookAttacksOnTheFly(square,occupancies[index]);
    }

    for(int randomCount=0; randomCount < 1000000000; randomCount++){
        Bitboard magicNumber = generateMagicNumber();
        if(popcount((occupancyMask * magicNumber) & 0xff00000000000000) < 6) continue;
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

void Position::initLeaperPiecesMoves() {
    for(int square=0; square<64; square++){
        pawnAttacks[White][square] = maskPawnAttacks(square,White);
        pawnAttacks[Black][square] = maskPawnAttacks(square,Black);
        knightAttacks[square] = maskKnightAttacks(square);
        kingAttacks[square] = maskKingAttacks(square);
    }
}

void Position::initTranspositionTable() {
    memset(transpositionTable, 0, sizeof(transpositionTable));
}

void Position::initSliderPiecesMoves(bool bishop) {
    for(int square=0; square < 64; square++){
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

int Position::generateRandomLegalMove() {
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

Bitboard Position::generateZobristHashKey(){
    Bitboard key = 0;
    Bitboard bitboard;
    for(int piece=WhitePawn; piece<=BlackKing; piece++){
        bitboard = pieceBoard[piece];
        while(bitboard){
            int square = lsb(bitboard);
            key ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+square];
            popBit(bitboard,square);
        }
    }
    if(castlingRights & 1) key ^= PolyglotRandomNumbers[ZH_CASTLING];
    if(castlingRights & 2) key ^= PolyglotRandomNumbers[ZH_CASTLING+1];
    if(castlingRights & 4) key ^= PolyglotRandomNumbers[ZH_CASTLING+2];
    if(castlingRights & 8) key ^= PolyglotRandomNumbers[ZH_CASTLING+3];

    if((enPassant != noSquare) && (pawnAttacks[1-toMove][enPassant] & pieceBoard[WhitePawn+6*toMove]))
        key ^= PolyglotRandomNumbers[ZH_EN_PASSANT+enPassant%8];
    
    if(toMove == White)
        key ^= PolyglotRandomNumbers[ZH_TURN];
    
    return key;
}

constexpr Bitboard Position::getBishopAttacks(unsigned int square, Bitboard occupancy) const{
    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopRelevantBits[square];
    return bishopAttacks[square][occupancy];
}

constexpr Bitboard Position::getRookAttacks(unsigned int square, Bitboard occupancy) const{
    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookRelevantBits[square];
    return rookAttacks[square][occupancy];
}

constexpr Bitboard Position::getQueenAttacks(unsigned int square, Bitboard occupancy) const{
    return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
}

int Position::getAllPossibleMoves(moves *moveList) {
    moveList->count = 0;
    int result = 0;
    Bitboard bitboard, attacks;
    unsigned int fromSquare, toSquare;
    for(unsigned int piece=WhitePawn; piece<=BlackKing; piece++){
        bitboard = pieceBoard[piece];
        if(toMove == White){
            if(piece == WhitePawn){
                while(bitboard){
                    fromSquare = lsb(bitboard);
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
                    if(enPassant != noSquare){
                        Bitboard enPassantAttacks = pawnAttacks[White][fromSquare] & (square_bb(enPassant));
                        if(enPassantAttacks){
                            unsigned int toEnpassant = lsb(enPassantAttacks);
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
                    fromSquare = lsb(bitboard);
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
                    if(enPassant != noSquare){
                        Bitboard enPassantAttacks = pawnAttacks[Black][fromSquare] & (square_bb(enPassant));
                        if(enPassantAttacks){
                            unsigned int toEnpassant = lsb(enPassantAttacks);
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
                fromSquare = lsb(bitboard);
                attacks = knightAttacks[fromSquare] & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = lsb(attacks);
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
                fromSquare = lsb(bitboard);
                attacks = getBishopAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = lsb(attacks);
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
                fromSquare = lsb(bitboard);
                attacks = getRookAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = lsb(attacks);
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
                fromSquare = lsb(bitboard);
                attacks = getQueenAttacks(fromSquare, occupiedBoard[Both]) & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = lsb(attacks);
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
                fromSquare = lsb(bitboard);
                attacks = kingAttacks[fromSquare] & 
                            ((toMove == White) ? ~occupiedBoard[White] : ~occupiedBoard[Black]);
                while(attacks){
                    toSquare = lsb(attacks);
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

Bitboard Position::getRandomBitboardnumber() const{
    Bitboard n1,n2,n3,n4;
    n1 = (Bitboard)(random() & 0xffff);
    n2 = (Bitboard)(random() & 0xffff);
    n3 = (Bitboard)(random() & 0xffff);
    n4 = (Bitboard)(random() & 0xffff);

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

Bitboard Position::generateMagicNumber() const{
    return getRandomBitboardnumber() & getRandomBitboardnumber() & getRandomBitboardnumber(); 
}

bool Position::isSquareAttacked(unsigned int square, ColorType color) const{
    if((color == White) && (pawnAttacks[Black][square] & pieceBoard[WhitePawn])) return true;
    if((color == Black) && (pawnAttacks[White][square] & pieceBoard[BlackPawn])) return true;
    if(knightAttacks[square] & ((color == White) ? pieceBoard[WhiteKnight] : pieceBoard[BlackKnight])) return true;
    if(getBishopAttacks(square,occupiedBoard[Both]) & pieceBoard[WhiteBishop + 6*color]) return true;
    if(getRookAttacks(square,occupiedBoard[Both]) & pieceBoard[WhiteRook + 6*color]) return true;
    if(getQueenAttacks(square,occupiedBoard[Both]) & pieceBoard[WhiteQueen + 6*color]) return true;
    if(kingAttacks[square] & pieceBoard[WhiteKing + 6*color]) return true;
    return false;
}

void Position::initMagicNumbers() const{
    for(int square=0; square<64; square++){
        printf(" 0x%lxULL,\n", findMagicNumber(square,bishopRelevantBits[square], true));
    }

    for(int square=0; square<64; square++){
        printf(" 0x%lxULL,\n", findMagicNumber(square,rookRelevantBits[square], false));
    }
}

constexpr void Position::initTables() {
    for (int piece = WhitePawn; piece <= WhiteKing; piece++) {
        for (int square = 0; square < 64; square++) {
            mg_table[piece][square] = mg_value[piece] + mg_pesto_table[piece][square];
            eg_table[piece][square] = eg_value[piece] + eg_pesto_table[piece][square];
            mg_table[piece+6][square] = mg_value[piece] + mg_pesto_table[piece][square^56];
            eg_table[piece+6][square] = eg_value[piece] + eg_pesto_table[piece][square^56];
        }
    }
}

int Position::makeMove(int move){
    copyBoard();
    unsigned int fromSquare = getFrom(move);
    unsigned int toSquare = getTo(move);
    Bitboard bbFromSquare = square_bb(Square(fromSquare));
    Bitboard bbToSquare   = square_bb(Square(toSquare));
    Bitboard bbFromToSquare = bbFromSquare ^ bbToSquare;
    int piece = getPiece(move);
    int promoted = getPromotedPiece(move);
    pieceBoard[piece] ^= bbFromToSquare;
    occupiedBoard[toMove] ^= bbFromToSquare;
    occupiedBoard[Both] ^= bbFromToSquare;
    ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+fromSquare];
    ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+toSquare];
    if(getCaptureFlag(move)){
        if(butterflyBoard[toSquare] != NoPiece){
            pieceBoard[butterflyBoard[toSquare]] ^= bbToSquare;
            occupiedBoard[1-toMove] ^= bbToSquare;
            occupiedBoard[Both] ^= bbToSquare;
            ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[butterflyBoard[toSquare]]+toSquare];
        }
    }
    butterflyBoard[fromSquare] = NoPiece;
    butterflyBoard[toSquare] = piece;
    if(promoted){
        pieceBoard[piece] ^= bbToSquare;
        pieceBoard[promoted] ^= bbToSquare;
        butterflyBoard[toSquare] = promoted;
        ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+toSquare];
        ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[promoted]+toSquare];
    }
    if(getEnPassantFlag(move)){
        if(toMove == White){
            popBit(pieceBoard[BlackPawn],(toSquare-8));
            popBit(occupiedBoard[Black],(toSquare-8));
            popBit(occupiedBoard[Both],(toSquare-8));
            butterflyBoard[toSquare-8] = NoPiece;
            ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackPawn]+toSquare-8];
        } else {
            popBit(pieceBoard[WhitePawn],(toSquare+8));
            popBit(occupiedBoard[White],(toSquare+8));
            popBit(occupiedBoard[Both],(toSquare+8));
            butterflyBoard[toSquare+8] = NoPiece;
            ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhitePawn]+toSquare+8];
        }         
    }
    enPassant = noSquare;
    
    if(didPolyglotFlipEnPassant){
        ZobristHashKey ^= PolyglotRandomNumbers[ZH_EN_PASSANT+enPassantCopy%8];
        didPolyglotFlipEnPassant = false;
    }

    if(getDoublePawnPushFlag(move)){
        enPassant = (Square)((toMove == White) ? toSquare - 8 : toSquare + 8);
        if((pawnAttacks[toMove][enPassant] & pieceBoard[BlackPawn-6*toMove])){
            ZobristHashKey ^= PolyglotRandomNumbers[ZH_EN_PASSANT+enPassant%8];
            didPolyglotFlipEnPassant = true;
        }
    }
 
    if(getCastlingFlag(move)){
        switch(toSquare){
            case g1:
                pieceBoard[WhiteRook] ^= h1f1;
                occupiedBoard[White] ^= h1f1;
                occupiedBoard[Both] ^= h1f1;
                butterflyBoard[h1] = NoPiece;
                butterflyBoard[f1] = WhiteRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+h1];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+f1];
                break;
            case c1:
                pieceBoard[WhiteRook] ^= a1d1;
                occupiedBoard[White]  ^= a1d1;
                occupiedBoard[Both]   ^= a1d1;
                butterflyBoard[a1] = NoPiece;
                butterflyBoard[d1] = WhiteRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+a1];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+d1];
                break;
            case g8:
                pieceBoard[BlackRook] ^= h8f8;
                occupiedBoard[Black]  ^= h8f8;
                occupiedBoard[Both]   ^= h8f8;
                butterflyBoard[h8] = NoPiece;
                butterflyBoard[f8] = BlackRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+h8];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+f8];
                break;
            case c8:
                pieceBoard[BlackRook] ^= a8d8;
                occupiedBoard[Black]  ^= a8d8;
                occupiedBoard[Both]   ^= a8d8;
                butterflyBoard[a8] = NoPiece;
                butterflyBoard[d8] = BlackRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+a8];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+d8];
                break;
        }     
    }
    castlingRights &= castlingRightsTable[fromSquare];
    castlingRights &= castlingRightsTable[toSquare];

    if(castlingRightsCopy != castlingRights){
        int change = castlingRightsCopy - castlingRights;
        switch(change){
            case 1:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING];
                break;
            case 2:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING+1];
                break;
            case 3:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING] ^ PolyglotRandomNumbers[ZH_CASTLING+1];
                break;
            case 4:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING+2];
                break;
            case 5:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING]^PolyglotRandomNumbers[ZH_CASTLING+2];
                break;
            case 8:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING+3];
                break;
            case 10:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING+1]^PolyglotRandomNumbers[ZH_CASTLING+3];
                break;
            case 12:
                ZobristHashKey ^= PolyglotRandomNumbers[ZH_CASTLING+2] ^ PolyglotRandomNumbers[ZH_CASTLING+3];
                break;
        }
    }
    
    toMove = (ColorType)(1-toMove);
    ZobristHashKey ^= PolyglotRandomNumbers[ZH_TURN];

    if(isSquareAttacked((toMove == White) ? lsb(pieceBoard[BlackKing]) : lsb(pieceBoard[WhiteKing]),toMove)){
            restoreBoard();
            return 0;
        }
    return 1; 
}

void Position::test() {
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

long long Position::perftDriver(int depth){
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

void Position::perftTestSuite(){
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
    return;
}

Bitboard Position::maskKingAttacks(unsigned int square){
    Bitboard attacks = 0;
    Bitboard king = square_bb(Square(square));

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

Bitboard Position::maskKnightAttacks(unsigned int square){
    Bitboard attacks = 0;
    Bitboard knight = square_bb(Square(square));

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

Bitboard Position::maskPawnAttacks(unsigned int square, ColorType color){
    Bitboard attacks = 0;
    Bitboard pawn = square_bb(Square(square));

    if(color == White){
        attacks |= pawn << 7 & notHFile;
        attacks |= pawn << 9 & notAFile;
    } else {
        attacks |= pawn >> 7 & notAFile;
        attacks |= pawn >> 9 & notHFile;
    }

    return attacks;
}

Bitboard Position::maskBishopOccupancy(unsigned int square) const{
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

Bitboard Position::maskRookOccupancy(unsigned int square) const{
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

constexpr Bitboard Position::generateBishopAttacksOnTheFly(unsigned int square, Bitboard block) const{
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

constexpr Bitboard Position::generateRookAttacksOnTheFly(unsigned int square, Bitboard block) const{
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

Square Position::parseSquare(std::string square){
    int file = square[0] - 'a';
    int rank = square[1] - '1';
    return (Square)(rank * 8 + file);
}

int Position::parseMove(std::string uciMove) {
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

void Position::parseGo(std::string go){
    std::vector<std::string> command = split(go," ");
    // dynamic time thinking
    if(command.size() > 1){
        if(toMove == White){
            timeToThink = atoi(command[2].c_str()) / 60000 + 1;
        } else {
            timeToThink = atoi(command[4].c_str()) / 60000 + 1;
        }
    } else {
        timeToThink = 5;
    }
    startSearch();
    printf("\n");
    std::cout << "bestmove ";
    printMoveUCI(PVTable[0][0]);
    std::cout << std::endl;
    //printMoveUCI(generateRandomLegalMove());
}

void Position::parsePosition(std::string position){
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

void Position::printBitBoard(Bitboard bitboard) const{
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

void Position::printMove(int move) const{
    std::cout << "   "  << squares[getFrom(move)] 
                        << squares[getTo(move)] 
                        << promotedPieces[getPromotedPiece(move)]
                        << "       " << unicodePieces[getPiece(move)]
                        << (getCaptureFlag(move) ? "   capture " : "")
                        << (getDoublePawnPushFlag(move) ? "    double pawn push " : "")
                        << (getEnPassantFlag(move) ? "    en passant " : "")
                        << (getCastlingFlag(move) ? "    castle" : "") << std::endl;
}

void Position::printMoveUCI(int move) const{
    std::cout << squares[getFrom(move)]
              << squares[getTo(move)];
    if(getPromotedPiece(move))
        std::cout << promotedPieces[getPromotedPiece(move)];
}

void Position::printMoveList(moves *moveList) const{
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

int Position::probeHash(int depth, int alpha, int beta, int *move){
    transpositionTableType *hashEntry = &transpositionTable[ZobristHashKey % hashTableSize];
    if(hashEntry->key == ZobristHashKey){
        if(hashEntry->depth >= depth){
            *move = hashEntry->bestMove;
            if(hashEntry->flag == EXACT)
                return hashEntry->evaluation;
            if(hashEntry->flag == ALPHA && hashEntry->evaluation <= alpha)
                return alpha;
            if(hashEntry->flag == BETA && hashEntry->evaluation >= beta)
                return beta;
        }
    }
    return UNKNOWN;
}

int Position::quiescienceSearch(int alpha, int beta){
    nodes++;
    // PVLength[ply] = ply;
    int currentEval = evaluatePosition();
    if(currentEval >= beta) return beta;
    if(currentEval > alpha) alpha = currentEval;
    moves moveList[1];
    getAllPossibleMoves(moveList);
    sortMoves(moveList);
    for(int i=0; i<moveList->count; i++){
        if(!getCaptureFlag(moveList->moves[i])) continue;
        copyBoard();
        if(!makeMove(moveList->moves[i])) continue;
        ply++;
        int evaluation = -quiescienceSearch(-beta, -alpha);
        ply--;
        restoreBoard();
        if(searchCancelled) return 0;
        if(evaluation >= beta){
            return beta;
        }
        if(evaluation > alpha){
            alpha = evaluation;
        }
    }
    return alpha;
}

void Position::recordHash(int depth, int evaluation, int hashFlag, int move){
    transpositionTableType *hashEntry = &transpositionTable[ZobristHashKey % hashTableSize];

    hashEntry->key = ZobristHashKey;
    hashEntry->depth = depth;
    hashEntry->evaluation = evaluation;
    hashEntry->bestMove = move;
    hashEntry->flag = hashFlag;
}

int Position::scoreMove(int move){
    if(move == PVTable[0][ply]) return 50000; //in tricky position it's worse but everywhere else it's better ?
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

int Position::searchBestMove(int depth, int alpha, int beta){
    nodes++;
    // bool foundPV = false;
    PVLength[ply] = ply;
    int evaluation;
    int hashFlag = ALPHA;
    // still bugged
    int bestMoveFromHash = 0;
    probeHash(depth, alpha, beta, &bestMoveFromHash);
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
        int currentMove = moveList->moves[i];
        if(bestMoveFromHash != 0){
            currentMove = bestMoveFromHash;
            bestMoveFromHash = 0;
            i--;
        }
        if(!makeMove(currentMove)) continue;
        legalMoves++;
        ply++;
        evaluation = -searchBestMove(depth-1, -beta, -alpha);
        ply--;
        restoreBoard();
        if(searchCancelled) return 0;
        if(evaluation >= beta){
            recordHash(depth, beta, BETA, currentMove);
            killerMoves[1][ply] = killerMoves[0][ply];
            killerMoves[0][ply] = currentMove;
            return beta;
        }
        if(evaluation > alpha){
            hashFlag = EXACT;
            // foundPV = true;
            recordHash(depth, alpha, hashFlag, currentMove);
            if(!getCaptureFlag(currentMove))
                historyMoves[getPiece(currentMove)][getTo(currentMove)] += depth; //remember this move as it improves our position
            alpha = evaluation;
            PVTable[ply][ply] = currentMove;
            for(int next = ply+1; next < PVLength[ply+1]; next++)
                PVTable[ply][next] = PVTable[ply+1][next];
            PVLength[ply] = PVLength[ply+1];
        }
    }

    if(legalMoves == 0){
        if(isSquareAttacked(lsb(pieceBoard[(toMove == White) ? WhiteKing : BlackKing]),(ColorType)(1-toMove))){
            return -50000+ply;
        }
        return 0;
    }

    return alpha;
}

Bitboard Position::setOccupancyBits(int index, int bitsInMask, Bitboard occupancy_mask) const{
    Bitboard occupancy = 0;

    for(int count=0; count<bitsInMask; count++){
        int square = lsb(occupancy_mask);
        popBit(occupancy_mask,square);
        if(index & (1 << count))
            occupancy |= square_bb(Square(square));
    }

    return occupancy;
}

void Position::sleep(int seconds){
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    searchCancelled = true;
}

void Position::sortMoves(moves *moveList){
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
    // std::sort(&scores[0], &scores[0] + moveList->count,[&](int a, int b) -> int {
    //             return a > b;
    //         });
    // printMoveScores(moveList, scores);
    // exit(0);
}

void Position::printMoveScores(moves *moveList, int *scores){
    printf("Debugging sort moves: \n");
    for(int i=0;i<moveList->count;i++){
        printf("move: ");
        printMoveUCI(moveList->moves[i]);
        printf(" score: %d\n", scores[i]);
    }
    printf("\n");
}

std::vector<std::string> Position::split(std::string s,std::string delimiter){
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

void Position::startSearch() {
    searchCancelled = false;
    maxPly = 0;
    std::thread sleeper(&Position::sleep, this, timeToThink);
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
        for(int i=0; i<6; i++){
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

void Position::UCImainLoop() {
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

void Position::visualizeBoard() const{
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

void Position::visualizeButterflyBoard() const{
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

void Position::FromFEN(std::string FEN){
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
    enPassant = noSquare;
    unsigned int rank = EIGHT_RANK;
    unsigned int index = 0;
    unsigned int position = 56;
    Bitboard pawn;
    Bitboard knight;
    Bitboard bishop;
    Bitboard rook;
    Bitboard queen;
    Bitboard king;
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
                rook = square_bb(Square(position));
                pieceBoard[BlackRook] ^= rook;
                occupiedBoard[Black] ^= rook;
                occupiedBoard[Both] ^= rook;
                butterflyBoard[position] = BlackRook;
                position += 1;
                break;
            case 'R':
                rook = square_bb(Square(position));
                pieceBoard[WhiteRook] ^= rook;
                occupiedBoard[White] ^= rook;
                occupiedBoard[Both] ^= rook;
                butterflyBoard[position] = WhiteRook;
                position += 1;
                break;
            case 'b':
                bishop = square_bb(Square(position));
                pieceBoard[BlackBishop] ^= bishop;
                occupiedBoard[Black] ^= bishop;
                occupiedBoard[Both] ^= bishop;
                butterflyBoard[position] = BlackBishop;
                position += 1;
                break;
            case 'B':
                bishop = square_bb(Square(position));
                pieceBoard[WhiteBishop] ^= bishop;
                occupiedBoard[White] ^= bishop;
                occupiedBoard[Both] ^= bishop;
                butterflyBoard[position] = WhiteBishop;
                position += 1;
                break;
            case 'n':
                knight = square_bb(Square(position));
                pieceBoard[BlackKnight] ^= knight;
                occupiedBoard[Black] ^= knight;
                occupiedBoard[Both] ^= knight;
                butterflyBoard[position] = BlackKnight;
                position += 1;
                break;
            case 'N':
                knight = square_bb(Square(position));
                pieceBoard[WhiteKnight] ^= knight;
                occupiedBoard[White] ^= knight;
                occupiedBoard[Both] ^= knight;
                butterflyBoard[position] = WhiteKnight;
                position += 1;
                break;
            case 'q':
                queen = square_bb(Square(position));
                pieceBoard[BlackQueen] ^= queen;
                occupiedBoard[Black] ^= queen;
                occupiedBoard[Both] ^= queen;
                butterflyBoard[position] = BlackQueen;
                position += 1;
                break;
            case 'Q':
                queen = square_bb(Square(position));
                pieceBoard[WhiteQueen] ^= queen;
                occupiedBoard[White] ^= queen;
                occupiedBoard[Both] ^= queen;
                butterflyBoard[position] = WhiteQueen;
                position += 1;
                break;
            case 'k':
                king = square_bb(Square(position));
                pieceBoard[BlackKing] ^= king;
                occupiedBoard[Black] ^= king;
                occupiedBoard[Both] ^= king;
                butterflyBoard[position] = BlackKing;
                position += 1;
                break;
            case 'K':
                king = square_bb(Square(position));
                pieceBoard[WhiteKing] ^= king;
                occupiedBoard[White] ^= king;
                occupiedBoard[Both] ^= king;
                butterflyBoard[position] = WhiteKing;
                position += 1;
                break;
            case 'p':
                pawn = square_bb(Square(position));
                pieceBoard[BlackPawn] ^= pawn;
                occupiedBoard[Black] ^= pawn;
                occupiedBoard[Both] ^= pawn;
                butterflyBoard[position] = BlackPawn;
                position += 1;
                break;
            case 'P':
                pawn = square_bb(Square(position));
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
    ZobristHashKey = generateZobristHashKey();
}

void Position::ZobristHashingTestSuite(){
    std::string correct = u8"✅";
    std::string incorrect = u8"❌";
    Bitboard key;
    FromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    key = generateZobristHashKey();
    std::cout << "Test 1: " << ((key == 0x463b96181691fc9c) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    key = generateZobristHashKey();
    std::cout << "Test 2: " << ((key == 0x823c9b50fd114196) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    key = generateZobristHashKey();
    std::cout << "Test 3: " << ((key == 0x0756b94461c50fb0) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    key = generateZobristHashKey();
    std::cout << "Test 4: " << ((key == 0x662fafb965db29d4) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    key = generateZobristHashKey();
    std::cout << "Test 5: " << ((key == 0x22a48b5a8e47ff78) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3");
    key = generateZobristHashKey();
    std::cout << "Test 6: " << ((key == 0x652a607ca3f242c1) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4");
    key = generateZobristHashKey();
    std::cout << "Test 7: " << ((key == 0x00fdd303c946bdd9) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3");
    key = generateZobristHashKey();
    std::cout << "Test 8: " << ((key == 0x3c8123ea7b067637) ? correct : incorrect) << " " << std::hex << key << std::endl;
    FromFEN("rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4");
    key = generateZobristHashKey();
    std::cout << "Test 9: " << ((key == 0x5c3f9b829b279560) ? correct : incorrect) << " " << std::hex << key << std::endl;
}

} // namespace EVal