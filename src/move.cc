#include "move.h"

Move::Move(unsigned int from, unsigned int to, unsigned int flags, ColorType color, PieceType piece): color(color), piece(piece){
    move = ((flags & 0xf) << 12)| ((from & 0x3f) << 6) | (to & 0x3f);
}

Move::Move(unsigned int from, unsigned int to, unsigned int flags, ColorType color, PieceType piece, PieceType capturedPiece): color(color), piece(piece), capturedPiece(capturedPiece){
    move = ((flags & 0xf) << 12)| ((from & 0x3f) << 6) | (to & 0x3f);
}

// unsigned int Move::getTo() const {
//     return move & 0x3f;
// }

// unsigned int Move::getFrom() const {
//     return ((move) >> 6) & 0x3f;
// }

unsigned int Move::getFlags() const{
    return (move >> 12) & 0xf;
}

PieceType Move::getPromotionPiece() const{
    unsigned int moveFlag = (move >> 12) & 0xf;
    switch(moveFlag){
        case KNIGHT_PROMOTION:
        case KNIGHT_PROMOTION_CAPTURE:
            return WhiteKnight;

        case BISHOP_PROMOTION:
        case BISHOP_PROMOTION_CAPTURE:
            return WhiteBishop;

        case ROOK_PROMOTION:
        case ROOK_PROMOTION_CAPTURE:
            return WhiteRook;

        case QUEEN_PROMOTION:
        case QUEEN_PROMOTION_CAPTURE:
            return WhiteQueen;

        default:
            throw;
    }
}


void Move::setTo(unsigned int to){
    move &= ~0x3f;
    move |= to & 0x3f;
}

void Move::setFrom(unsigned int from){
    move &= ~0xfc0;
    move |= (from & 0x3f) << 6;
}

void Move::setFlags(unsigned int flags){
    move &= ~0xf000;
    move |= (flags & 0xf) << 12;
}

bool Move::isCapture() const {
    return (move & CAPTURE_FLAG) != 0;
}

MoveType Move::getMoveType() const{
    unsigned int result = (move & CAPTURE_FLAG) 
                        | (move & PROMOTION_FLAG) 
                        | (move & SPECIAL_FLAG_1)
                        | (move & SPECIAL_FLAG_0);
    return MoveType(result >> 12);
}

void Move::operator=(Move toCopy){
    this->move = toCopy.move;
}

bool Move::operator==(Move toCompare) const {
    return (move & 0xffff) == (toCompare.move & 0xffff);
}

bool Move::operator!=(Move toCompare) const {
    return (move & 0xffff) != (toCompare.move & 0xffff);
}

// std::ostream &operator<<(std::ostream& os, const Move &move){
//     return os << "Move " << move.getFrom() << std::endl;
// }
