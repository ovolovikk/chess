#include "Board.hpp"

Board::Board() {
    setupStartingPosition();
}

Piece Board::getPiece(Position pos) const {
    if (!isInside(pos)) return {PieceType::None, Color::None};
    return squares[pos.row][pos.col];
}

void Board::setPiece(Position pos, Piece piece) {
    if (isInside(pos)) {
        squares[pos.row][pos.col] = piece;
    }
}

void Board::movePiece(Position from, Position to) {
    Piece p = getPiece(from);
    setPiece(to, p);
    setPiece(from, {PieceType::None, Color::None});
}

bool Board::isInside(Position pos) const {
    return pos.row >= 0 && pos.row < 8 && pos.col >= 0 && pos.col < 8;
}

bool Board::isEmpty(Position pos) const {
    return getPiece(pos).type == PieceType::None;
}

void Board::setupStartingPosition() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            squares[r][c] = {PieceType::None, Color::None};
        }
    }

    // Pawns
    for (int c = 0; c < 8; ++c) {
        setPiece({1, c}, {PieceType::Pawn, Color::Black});
        setPiece({6, c}, {PieceType::Pawn, Color::White});
    }

    // Back rank pieces
    PieceType backRank[] = {PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen, PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook};
    for (int c = 0; c < 8; ++c) {
        setPiece({0, c}, {backRank[c], Color::Black});
        setPiece({7, c}, {backRank[c], Color::White});
    }
}