#pragma once
#include "Types.hpp"
#include <array>

class Board {
public:
    Board();
    
    Piece getPiece(Position pos) const;
    void setPiece(Position pos, Piece piece);

    bool isInside(Position pos) const;
    bool isEmpty(Position pos) const;

    bool makeMove(const Move& move);
    void setupStartingPosition();

private:
    std::array<std::array<Piece, 8>, 8> squares;
};