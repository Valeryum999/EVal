#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cassert>

namespace EVal {

using Bitboard = std::uint64_t;

enum Color : uint8_t {
    WHITE,
    BLACK,
    BOTH = 2
};

enum PieceType : std::uint8_t{
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Piece : std::uint8_t{
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
    NoPiece
};

enum Square : uint8_t{
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    noSquare
};

constexpr Square& operator++(Square& s) { return s = Square(int(s) + 1); }
constexpr Square operator+(Square s, int i) { return Square(int(s) + i); }
constexpr Square operator-(Square s, int i) { return Square(int(s) - i); }
constexpr Square& operator+=(Square& s, int i) { return s = s + i; }
constexpr Square& operator-=(Square& s, int i) { return s = s - i; }

constexpr Piece& operator++(Piece& p) { return p = Piece(int(p) + 1); }

constexpr bool is_ok(Square s) { return s >= a1 && s <= h8; }

enum MoveType {
    NORMAL,
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
};

// 16 bit to store a move
// bit 0-5 (6 bits)   : destination square
// bit 6-11 (6 bits)  : source square
// bit 12-13 (2 bits) : promotion piece type
// bit 14-15 (2 bits) : special movetype flag: promotion(1), enpassant(2), castling(3)

class Move{
   protected:
    std::uint16_t data;
   public:
    Move() = default;
    constexpr explicit Move(std::uint16_t d) :
        data(d) {}
    
    constexpr Move(Square from, Square to) :
        data((from << 6) + to) {} 
    
    template<MoveType T>
    static constexpr Move make(Square from, Square to, PieceType pt = KNIGHT){
        return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

    constexpr Square from_sq() const {
        assert(is_ok());
        return Square((data >> 6) & 0x3F);
    }

    constexpr Square to_sq() const {
        assert(is_ok());
        return Square(data & 0x3F);
    }

    constexpr int from_to() const { return data & 0xFFF; }
    
    static constexpr Move null() { return Move(65); }
    static constexpr Move none() { return Move(0); }

    constexpr bool is_ok() const { return none().data != data && null().data != data; }

    constexpr bool operator==(const Move& m) const { return data == m.data; }
    constexpr bool operator!=(const Move& m) const { return data != m.data; }

    constexpr std::uint16_t raw() const { return data; }
};

} // EVal namespace
#endif 