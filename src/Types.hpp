#pragma once

enum class Color { White, Black, None };
enum class PieceType { None, Pawn, Knight, Bishop, Rook, Queen, King };

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::None;
};

struct Position {
    int row = 0;
    int col = 0;

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

struct Move {
    Position from;
    Position to;
};
