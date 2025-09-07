#ifndef MOVE_H
#define MOVE_H

#include "constants.h"
#include <iostream>

enum MoveFlags{
    SPECIAL_FLAG_0 = 0x1000,
    SPECIAL_FLAG_1 = 0x2000,
    CAPTURE_FLAG = 0x4000,
    PROMOTION_FLAG = 0x8000,
};

enum MoveType{
    QUIET_MOVE,
    DOUBLE_PAWN_PUSH,
    KING_CASTLE,
    QUEEN_CASTLE,
    CAPTURE,
    EN_PASSANT_CAPTURE,
    KNIGHT_PROMOTION = 8,
    BISHOP_PROMOTION,
    ROOK_PROMOTION,
    QUEEN_PROMOTION,
    KNIGHT_PROMOTION_CAPTURE,
    BISHOP_PROMOTION_CAPTURE,
    ROOK_PROMOTION_CAPTURE,
    QUEEN_PROMOTION_CAPTURE,
};

class Move {
    public:
        unsigned int move;
        ColorType color;
        PieceType piece;
        PieceType capturedPiece;
        PieceType promotionPiece;

        Move(unsigned int from, unsigned int to, unsigned int flags, ColorType color, PieceType piece);
        Move(unsigned int from, unsigned int to, unsigned int flags, ColorType color, PieceType piece, PieceType capturedPiece);

        // unsigned int getFrom() const;
        // unsigned int getTo() const;
        unsigned int getFlags() const;
        PieceType getPromotionPiece() const;

        void setFrom(unsigned int from);
        void setTo(unsigned int to);
        void setFlags(unsigned int flags);

        bool isCapture() const;
        MoveType getMoveType() const;

        void operator=(Move toCopy);
        bool operator==(Move toCompare) const;
        bool operator!=(Move toCompare) const;
        // friend std::ostream& operator<<(std::ostream& ,const Move& move);
};

#endif