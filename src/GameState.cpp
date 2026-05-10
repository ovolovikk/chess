#include "GameState.hpp"

void GameState::selectSquare(Position pos) {
    if (!board.isInside(pos)) return;

    if (!selectedSquare.has_value()) {
        Piece p = board.getPiece(pos);
        if (p.color == sideToMove) {
            selectedSquare = pos;
        }
    } else {
        if (!tryMove(pos)) {
            // Re-select if another friendly piece is clicked
            Piece p = board.getPiece(pos);
            if (p.color == sideToMove) {
                selectedSquare = pos;
            } else {
                selectedSquare = std::nullopt;
            }
        }
    }
}

bool GameState::tryMove(Position to) {
    if (!selectedSquare.has_value()) return false;

    Move move{*selectedSquare, to};
    if (MoveGenerator::isLegalMove(board, move, sideToMove)) {
        board.makeMove(move);
        toggleSideToMove();
        selectedSquare = std::nullopt;
        return true;
    }
    
    selectedSquare = std::nullopt;
    return false;
}

void GameState::toggleSideToMove() {
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
}