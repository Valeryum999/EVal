#include "position.h"
#include "movegen.h"
#include "uci.h"

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
    occupiedBoard[WHITE]      = 0x0000000000000000;
    occupiedBoard[BLACK]      = 0x0000000000000000;
    occupiedBoard[BOTH]       = 0x0000000000000000;
    for(int square=a1; square<=h8; square++) butterflyBoard[square] = NoPiece;
    castlingRights = 0;
    canEnPassant[WHITE] = false;
    canEnPassant[BLACK] = false;
    enPassant = noSquare;
    toMove = WHITE;
    MoveGen::initLeaperPiecesMoves();
    MoveGen::initSliderPiecesMoves(false);
    MoveGen::initSliderPiecesMoves(true);
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

int Position::evaluatePosition(){
    int middlegame[2] = {0,0};
    int endgame[2] = {0,0};
    int mobility[2] = {0,0};
    int gamePhase = 0;

    for(Square square=a1; square <= h8; ++square){
        int piece = butterflyBoard[square];
        if(piece == NoPiece) continue;
        middlegame[(piece - 6) < 0 ? WHITE : BLACK] += mg_table[piece][squareToVisualBoardSquare[square]];
        endgame[(piece - 6) < 0 ? WHITE : BLACK] += eg_table[piece][squareToVisualBoardSquare[square]];
        gamePhase += gamephaseInc[piece];
        switch(piece){
            case WhitePawn:
            case BlackPawn:
                break;
            case WhiteKnight:
            case BlackKnight:
                if(piece == WhiteKnight)
                    mobility[WHITE] += popcount(MoveGen::knightAttacks[square] & ~occupiedBoard[WHITE]);
                else
                    mobility[BLACK] += popcount(MoveGen::knightAttacks[square] & ~occupiedBoard[BLACK]);
                break;
            case WhiteBishop:
            case BlackBishop:
                if(piece == WhiteBishop)
                    mobility[WHITE] += popcount(MoveGen::attacks_bb<BISHOP>(square, occupiedBoard[BOTH]) & ~occupiedBoard[WHITE]);
                else
                    mobility[BLACK] += popcount(MoveGen::attacks_bb<BISHOP>(square, occupiedBoard[BOTH]) & ~occupiedBoard[BLACK]);
                break;
            case WhiteRook:
            case BlackRook:
                if(piece == WhiteRook)
                    mobility[WHITE] += popcount(MoveGen::attacks_bb<ROOK>(square, occupiedBoard[BOTH]) & ~occupiedBoard[WHITE]);
                else
                    mobility[BLACK] += popcount(MoveGen::attacks_bb<ROOK>(square, occupiedBoard[BOTH]) & ~occupiedBoard[BLACK]);
                break;
            case WhiteQueen:
            case BlackQueen:
                if(piece == WhiteQueen)
                    mobility[WHITE] += popcount(MoveGen::attacks_bb<QUEEN>(square,occupiedBoard[BOTH]) & ~occupiedBoard[WHITE]);
                else
                    mobility[BLACK] += popcount(MoveGen::attacks_bb<QUEEN>(square,occupiedBoard[BOTH]) & ~occupiedBoard[BLACK]);
                break;
            case WhiteKing:
            case BlackKing:
                if(piece == WhiteKing)
                    mobility[WHITE] -= popcount(MoveGen::kingAttacks[square] & ~occupiedBoard[WHITE]);
                else
                    mobility[BLACK] -= popcount(MoveGen::kingAttacks[square] & ~occupiedBoard[BLACK]);
                break;
        }
    }

    // double, blocked or isolated pawns
    // Bitboard our_pawns   = pieces(PAWN, to_move());
    // Bitboard their_pawns = pieces(PAWN, 1-to_move());

    // int pawnStructureScore = 0;

    // while(our_pawns){
    //     Bitboard our_pawn = our_pawns ^ (our_pawns - 1);
    //     if(to_move() == WHITE){
    //         if(our_pawn & pieces(PAWN, WHITE)){

    //         }
    //     } else {

    //     }

    //     our_pawns ^= our_pawn;
    // }

    int middlegameScore = middlegame[toMove] - middlegame[1-toMove];
    int endgameScore = endgame[toMove] - endgame[1-toMove];
    int mobilityScore = mobility[toMove] - mobility[1-toMove];
    int middlegamePhase = gamePhase;
    if (middlegamePhase > 24) middlegamePhase = 24; /* in case of early promotion */
    int endgamePhase = 24 - middlegamePhase;
    return (middlegameScore * middlegamePhase + endgameScore * endgamePhase) / 24 + mobilityScore;
}

Bitboard Position::findMagicNumber(Square square, int relevantBits, bool isBishop) const{
    Bitboard occupancies[4096];
    Bitboard attacks[4096];
    Bitboard usedAttacks[4096];

    Bitboard occupancyMask = isBishop ? MoveGen::maskBishopOccupancy(square) 
                                      : MoveGen::maskRookOccupancy(square);

    int occupancyIndices = 1 << relevantBits;

    for(int index=0; index<occupancyIndices; index++){
        occupancies[index] = MoveGen::setOccupancyBits(index, relevantBits, occupancyMask);
        attacks[index] = isBishop ? MoveGen::generateBishopAttacksOnTheFly(square,occupancies[index]) 
                                  : MoveGen::generateRookAttacksOnTheFly(square,occupancies[index]);
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

void Position::initTranspositionTable() {
    memset(transpositionTable, 0, sizeof(transpositionTable));
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

    if((enPassant != noSquare) && (MoveGen::pawnAttacks[1-toMove][enPassant] & pieceBoard[WhitePawn+6*toMove]))
        key ^= PolyglotRandomNumbers[ZH_EN_PASSANT+enPassant%8];
    
    if(toMove == WHITE)
        key ^= PolyglotRandomNumbers[ZH_TURN];
    
    return key;
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

bool Position::isSquareAttacked(Square square, Color color) const{
    if((color == WHITE) && (MoveGen::pawnAttacks[BLACK][square] & pieceBoard[WhitePawn])) return true;
    if((color == BLACK) && (MoveGen::pawnAttacks[WHITE][square] & pieceBoard[BlackPawn])) return true;
    if(MoveGen::knightAttacks[square] & ((color == WHITE) ? pieceBoard[WhiteKnight] : pieceBoard[BlackKnight])) return true;
    if(MoveGen::attacks_bb<BISHOP>(square,occupiedBoard[BOTH]) & pieceBoard[WhiteBishop + 6*color]) return true;
    if(MoveGen::attacks_bb<ROOK>(square,occupiedBoard[BOTH]) & pieceBoard[WhiteRook + 6*color]) return true;
    if(MoveGen::attacks_bb<QUEEN>(square,occupiedBoard[BOTH]) & pieceBoard[WhiteQueen + 6*color]) return true;
    if(MoveGen::kingAttacks[square] & pieceBoard[WhiteKing + 6*color]) return true;
    return false;
}

void Position::initMagicNumbers() const{
    for(Square square=a1; square<h8; ++square){
        printf(" 0x%lxULL,\n", findMagicNumber(square,bishopRelevantBits[square], true));
    }

    for(Square square=a1; square<h8; ++square){
        printf(" 0x%lxULL,\n", findMagicNumber(square,rookRelevantBits[square], false));
    }
}

void Position::initTables() {
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
    Square fromSquare = Square(getFrom(move));
    Square toSquare = Square(getTo(move));
    Bitboard bbFromSquare = square_bb(fromSquare);
    Bitboard bbToSquare   = square_bb(toSquare);
    Bitboard bbFromToSquare = bbFromSquare ^ bbToSquare;
    int piece = getPiece(move);
    int promoted = getPromotedPiece(move);
    pieceBoard[piece] ^= bbFromToSquare;
    occupiedBoard[toMove] ^= bbFromToSquare;
    occupiedBoard[BOTH] ^= bbFromToSquare;
    ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+fromSquare];
    ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[piece]+toSquare];
    if(getCaptureFlag(move)){
        if(butterflyBoard[toSquare] != NoPiece){
            pieceBoard[butterflyBoard[toSquare]] ^= bbToSquare;
            occupiedBoard[1-toMove] ^= bbToSquare;
            occupiedBoard[BOTH] ^= bbToSquare;
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
        if(toMove == WHITE){
            popBit(pieceBoard[BlackPawn],(toSquare-8));
            popBit(occupiedBoard[BLACK],(toSquare-8));
            popBit(occupiedBoard[BOTH],(toSquare-8));
            butterflyBoard[toSquare-8] = NoPiece;
            ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackPawn]+toSquare-8];
        } else {
            popBit(pieceBoard[WhitePawn],(toSquare+8));
            popBit(occupiedBoard[WHITE],(toSquare+8));
            popBit(occupiedBoard[BOTH],(toSquare+8));
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
        enPassant = (Square)((toMove == WHITE) ? toSquare - 8 : toSquare + 8);
        if((MoveGen::pawnAttacks[toMove][enPassant] & pieceBoard[BlackPawn-6*toMove])){
            ZobristHashKey ^= PolyglotRandomNumbers[ZH_EN_PASSANT+enPassant%8];
            didPolyglotFlipEnPassant = true;
        }
    }
 
    if(getCastlingFlag(move)){
        switch(toSquare){
            case g1:
                pieceBoard[WhiteRook] ^= h1f1;
                occupiedBoard[WHITE] ^= h1f1;
                occupiedBoard[BOTH] ^= h1f1;
                butterflyBoard[h1] = NoPiece;
                butterflyBoard[f1] = WhiteRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+h1];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+f1];
                break;
            case c1:
                pieceBoard[WhiteRook] ^= a1d1;
                occupiedBoard[WHITE]  ^= a1d1;
                occupiedBoard[BOTH]   ^= a1d1;
                butterflyBoard[a1] = NoPiece;
                butterflyBoard[d1] = WhiteRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+a1];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[WhiteRook]+d1];
                break;
            case g8:
                pieceBoard[BlackRook] ^= h8f8;
                occupiedBoard[BLACK]  ^= h8f8;
                occupiedBoard[BOTH]   ^= h8f8;
                butterflyBoard[h8] = NoPiece;
                butterflyBoard[f8] = BlackRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+h8];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+f8];
                break;
            case c8:
                pieceBoard[BlackRook] ^= a8d8;
                occupiedBoard[BLACK]  ^= a8d8;
                occupiedBoard[BOTH]   ^= a8d8;
                butterflyBoard[a8] = NoPiece;
                butterflyBoard[d8] = BlackRook;
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+a8];
                ZobristHashKey ^= PolyglotRandomNumbers[64*PolyglotKindOfPiece[BlackRook]+d8];
                break;
            default:
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
    
    toMove = (Color)(1-toMove);
    ZobristHashKey ^= PolyglotRandomNumbers[ZH_TURN];

    if(isSquareAttacked((toMove == WHITE) ?
                          lsb(pieceBoard[BlackKing])
                        : lsb(pieceBoard[WhiteKing]),
                        toMove)){
            restoreBoard();
            return 0;
        }
    return 1; 
}

void Position::test() {
    moves moveList[1];
    // getAllPossibleMoves(moveList);
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
    MoveGen::getAllPossibleMoves(*this, moveList);
    // printMoveList(moveList);
    int partial_result = 0;
    for(int i=0; i<moveList->count; i++){
        copyBoard();
        if(!makeMove(moveList->moves[i])) continue;
        partial_result = perftDriver(depth-1);
        // if(depth == 1){
        //     printMoveUCI(moveList->moves[i]);
        //     printf(": %d\n", partial_result);
        // }
        nodes += partial_result;
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

void Position::printBitBoard(Bitboard bitboard) const{
    std::cout << std::endl << std::endl;
    for(int rank=7; rank>=0; rank--){
        std::cout << "  " << rank+1 << "  ";
        for(int file=0; file<8; file++){
            std::cout << getBit(bitboard, Square(rank*8+file))  << " ";
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
    MoveGen::getAllPossibleMoves(*this, moveList);
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

const int FullDepthMoves = 4;
const int ReductionLimit = 3;

int Position::searchBestMove(int depth, int alpha, int beta){ //2879445
    nodes++;
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
    bool is_in_check = isSquareAttacked(lsb(pieceBoard[WhiteKing + 6*toMove]), ~toMove);
    // null move pruning
    // if(depth >= 3 && !is_in_check && ply){
    //     copyBoard();
    //     toMove = ~toMove;
    //     enPassant = noSquare;
    //     evaluation = -searchBestMove(depth - 1 - 2, -beta, -beta + 1);
    //     restoreBoard();
    //     if(evaluation >= beta)
    //         return beta;
    // }

    moves moveList[1];
    moveList->count = 0;
    MoveGen::getAllPossibleMoves(*this, moveList);
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
        // if first move, expected PVS
        if(i==0){
            evaluation = -searchBestMove(depth-1, -beta, -alpha);
        } else{
            //LMR
            if(i >= FullDepthMoves &&
               depth >= ReductionLimit &&
               !is_in_check &&
               !getCaptureFlag(moveList->moves[i]) &&
               !getPromotedPiece(moveList->moves[i])
            )
                evaluation = -searchBestMove(depth-2, -alpha - 1, -alpha);
            else evaluation = alpha + 1;

            //if found a better move during LMR
            if(evaluation > alpha){
                // research with full depth but narrower range
                evaluation = -searchBestMove(depth-1, -alpha - 1, -alpha);

                // if current move still seems better than the first re-search it as normal
                if((evaluation > alpha) && (evaluation < beta))
                    evaluation = -searchBestMove(depth-1, -beta, -alpha); 
            }
        }
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
        if(isSquareAttacked(lsb(pieceBoard[(toMove == WHITE) 
                            ? WhiteKing 
                            : BlackKing]),
                            (Color)(1-toMove))){
            return -50000+ply;
        }
        return 0;
    }

    return alpha;
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
        UCI::printMoveUCI(moveList->moves[i]);
        printf(" score: %d\n", scores[i]);
    }
    printf("\n");
}

void Position::startSearch() {
    searchCancelled = false;
    maxPly = 0;
    std::thread sleeper(&Position::sleep, this, timeToThink);
    sleeper.detach();
    int bestEvalSoFar = 0;
    int PVLengthCopy[64];
    int PVTableCopy[64][64];
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(PVTable, 0, sizeof(PVTable));
    memset(PVLength, 0, sizeof(PVLength));
    auto start_time = std::chrono::high_resolution_clock::now();
    int alpha = -50000;
    int beta = 50000;
    for(searchDepth = 1; searchDepth < 64; searchDepth++){
        nodes = 0;
        bestEval = searchBestMove(searchDepth, alpha, beta);
        
        if(searchCancelled) {
            printf("Stopped search at depth: %d\n", searchDepth);
            break;
        }

        // if(bestEval < alpha || bestEval > beta){
        //     alpha = -50000;
        //     beta = 50000;
        //     searchDepth--;
        //     continue;
        // }

        // // aspiration window
        // alpha = bestEval - 50;
        // beta = bestEval + 50;

        printf("info score cp %d depth %d nodes %lld pv", bestEval, searchDepth, nodes);
        for(int i=0; i<PVLength[0]; i++){
            printf(" ");
            UCI::printMoveUCI(PVTable[0][i]);
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

void Position::visualizeBoard() const{
    std::string renderBoard[8][32];
    for(int rank=0; rank<8; rank++){
        for(int file=0; file<8; file++){
            if(getBit(pieceBoard[WhitePawn],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whitePawn;
                continue;
            }
            if(getBit(pieceBoard[BlackPawn],Square(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackPawn;
                continue;
            }
            if(getBit(pieceBoard[WhiteKnight],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteKnight;
                continue;
            }
            if(getBit(pieceBoard[BlackKnight],Square(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackKnight;
                continue;
            }
            if(getBit(pieceBoard[WhiteBishop],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteBishop;
                continue;
            }
            if(getBit(pieceBoard[BlackBishop],Square(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackBishop;
                continue;
            }
            if(getBit(pieceBoard[WhiteRook],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteRook;
                continue;
            }
            if(getBit(pieceBoard[BlackRook],Square(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackRook;
                continue;
            }
            if(getBit(pieceBoard[WhiteQueen],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteQueen;
                continue;
            }
            if(getBit(pieceBoard[BlackQueen],Square(rank*8 + file)) == 1){
                renderBoard[rank][file] = blackQueen;
                continue;
            }
            if(getBit(pieceBoard[WhiteKing],Square(rank*8 + file)) == 1) {
                renderBoard[rank][file] = whiteKing;
                continue;
            }
            if(getBit(pieceBoard[BlackKing],Square(rank*8 + file)) == 1){
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
    occupiedBoard[WHITE]      = 0x0000000000000000;
    occupiedBoard[BLACK]      = 0x0000000000000000;
    occupiedBoard[BOTH]       = 0x0000000000000000;
    for(int square=a1; square<=h8; square++) butterflyBoard[square] = NoPiece;
    castlingRights = 0;
    enPassant = noSquare;
    unsigned int rank = EIGHT_RANK;
    unsigned int index = 0;
    Square position = a8;
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
                rook = square_bb(position);
                pieceBoard[BlackRook] ^= rook;
                occupiedBoard[BLACK] ^= rook;
                occupiedBoard[BOTH] ^= rook;
                butterflyBoard[position] = BlackRook;
                position += 1;
                break;
            case 'R':
                rook = square_bb(position);
                pieceBoard[WhiteRook] ^= rook;
                occupiedBoard[WHITE] ^= rook;
                occupiedBoard[BOTH] ^= rook;
                butterflyBoard[position] = WhiteRook;
                position += 1;
                break;
            case 'b':
                bishop = square_bb(position);
                pieceBoard[BlackBishop] ^= bishop;
                occupiedBoard[BLACK] ^= bishop;
                occupiedBoard[BOTH] ^= bishop;
                butterflyBoard[position] = BlackBishop;
                position += 1;
                break;
            case 'B':
                bishop = square_bb(position);
                pieceBoard[WhiteBishop] ^= bishop;
                occupiedBoard[WHITE] ^= bishop;
                occupiedBoard[BOTH] ^= bishop;
                butterflyBoard[position] = WhiteBishop;
                position += 1;
                break;
            case 'n':
                knight = square_bb(position);
                pieceBoard[BlackKnight] ^= knight;
                occupiedBoard[BLACK] ^= knight;
                occupiedBoard[BOTH] ^= knight;
                butterflyBoard[position] = BlackKnight;
                position += 1;
                break;
            case 'N':
                knight = square_bb(position);
                pieceBoard[WhiteKnight] ^= knight;
                occupiedBoard[WHITE] ^= knight;
                occupiedBoard[BOTH] ^= knight;
                butterflyBoard[position] = WhiteKnight;
                position += 1;
                break;
            case 'q':
                queen = square_bb(position);
                pieceBoard[BlackQueen] ^= queen;
                occupiedBoard[BLACK] ^= queen;
                occupiedBoard[BOTH] ^= queen;
                butterflyBoard[position] = BlackQueen;
                position += 1;
                break;
            case 'Q':
                queen = square_bb(position);
                pieceBoard[WhiteQueen] ^= queen;
                occupiedBoard[WHITE] ^= queen;
                occupiedBoard[BOTH] ^= queen;
                butterflyBoard[position] = WhiteQueen;
                position += 1;
                break;
            case 'k':
                king = square_bb(position);
                pieceBoard[BlackKing] ^= king;
                occupiedBoard[BLACK] ^= king;
                occupiedBoard[BOTH] ^= king;
                butterflyBoard[position] = BlackKing;
                position += 1;
                break;
            case 'K':
                king = square_bb(position);
                pieceBoard[WhiteKing] ^= king;
                occupiedBoard[WHITE] ^= king;
                occupiedBoard[BOTH] ^= king;
                butterflyBoard[position] = WhiteKing;
                position += 1;
                break;
            case 'p':
                pawn = square_bb(position);
                pieceBoard[BlackPawn] ^= pawn;
                occupiedBoard[BLACK] ^= pawn;
                occupiedBoard[BOTH] ^= pawn;
                butterflyBoard[position] = BlackPawn;
                position += 1;
                break;
            case 'P':
                pawn = square_bb(position);
                pieceBoard[WhitePawn] ^= pawn;
                occupiedBoard[WHITE] ^= pawn;
                occupiedBoard[BOTH] ^= pawn;
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
    toMove = (FEN[index] == 'w') ? WHITE : BLACK;
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
        enPassant = UCI::parseSquare(FEN.c_str()+index);
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