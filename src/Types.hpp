#pragma once
#include <optional>

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

enum class MoveType {
    Normal,
    Capture,
    DoublePawnPush,
    EnPassant,
    Castling,
    Promotion,
    PromotionCapture
};

struct Move {
    Position from;
    Position to;
    MoveType type = MoveType::Normal;
    PieceType promotionPiece = PieceType::None;

    bool operator==(const Move& other) const {
        return from == other.from &&
               to == other.to &&
               type == other.type &&
               promotionPiece == other.promotionPiece;
    }
};

struct CastlingRights {
    bool whiteKingSide = true;
    bool whiteQueenSide = true;
    bool blackKingSide = true;
    bool blackQueenSide = true;
};

inline Color opposite(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}
