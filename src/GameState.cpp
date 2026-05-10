#include "GameState.hpp"
#include "MoveGenerator.hpp"
#include <algorithm>

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

    // Generate ALL legal moves for the selected piece
    MoveGenerator gen;
    auto legalMoves = gen.generateLegalMoves(*this, *selectedSquare);
    
    // Find if the target square matches any legal move
    // Note: If multiple (e.g. promotions), we default to Queen promotion for quick UI testing here
    // A proper UI would ask the user, but for now we just pick the first valid matching 'to'
    for (const Move& legalMove : legalMoves) {
        if (legalMove.to == to) {
            makeMove(legalMove);
            updateStatus();
            selectedSquare = std::nullopt;
            return true;
        }
    }
    
    selectedSquare = std::nullopt;
    return false;
}

void GameState::makeMoveWithoutValidation(const Move& move) {
    Piece movingPiece = board.getPiece(move.from);

    switch (move.type) {
    case MoveType::Normal:
    case MoveType::Capture:
        board.movePiece(move.from, move.to);
        break;

    case MoveType::DoublePawnPush:
        board.movePiece(move.from, move.to);
        updateEnPassantTarget(move);
        break;

    case MoveType::EnPassant:
        applyEnPassant(move);
        break;

    case MoveType::Castling:
        applyCastling(move);
        break;

    case MoveType::Promotion:
    case MoveType::PromotionCapture:
        applyPromotion(move);
        break;
    }

    if (move.type != MoveType::DoublePawnPush) {
        enPassantTarget = std::nullopt; // Reset on other moves
    }

    updateCastlingRights(move, movingPiece);
    sideToMove = opposite(sideToMove);
}

bool GameState::makeMove(const Move& move) {
    // We assume the caller validated the move. Just apply.
    makeMoveWithoutValidation(move);
    
    if (sideToMove == Color::White) {
        fullmoveNumber++;
    }
    return true;
}

void GameState::applyEnPassant(const Move& move) {
    board.movePiece(move.from, move.to);
    // captured pawn is behind the 'to' square from the moving pawn's perspective
    int currRow = (sideToMove == Color::White) ? move.to.row + 1 : move.to.row - 1;
    board.setPiece({currRow, move.to.col}, {PieceType::None, Color::None});
}

void GameState::applyCastling(const Move& move) {
    board.movePiece(move.from, move.to);
    // Move the corresponding rook
    if (move.to.col == 6) { // Kingside
        board.movePiece({move.from.row, 7}, {move.from.row, 5});
    } else if (move.to.col == 2) { // Queenside
        board.movePiece({move.from.row, 0}, {move.from.row, 3});
    }
}

void GameState::applyPromotion(const Move& move) {
    board.movePiece(move.from, move.to);
    board.setPiece(move.to, {move.promotionPiece, sideToMove});
}

void GameState::updateEnPassantTarget(const Move& move) {
    int dir = (sideToMove == Color::White) ? -1 : 1;
    enPassantTarget = Position{move.from.row + dir, move.from.col};
}

void GameState::updateCastlingRights(const Move& move, Piece movingPiece) {
    if (movingPiece.type == PieceType::King) {
        if (sideToMove == Color::White) {
            castlingRights.whiteKingSide = false;
            castlingRights.whiteQueenSide = false;
        } else {
            castlingRights.blackKingSide = false;
            castlingRights.blackQueenSide = false;
        }
    }
    else if (movingPiece.type == PieceType::Rook) {
        if (sideToMove == Color::White) {
            if (move.from.col == 0) castlingRights.whiteQueenSide = false;
            if (move.from.col == 7) castlingRights.whiteKingSide = false;
        } else {
            if (move.from.col == 0) castlingRights.blackQueenSide = false;
            if (move.from.col == 7) castlingRights.blackKingSide = false;
        }
    }
}

void GameState::updateStatus() {
    MoveGenerator gen;
    bool hasMoves = false;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board.getPiece({r, c}).color == sideToMove) {
                if (!gen.generateLegalMoves(*this, {r, c}).empty()) {
                    hasMoves = true;
                    break;
                }
            }
        }
        if (hasMoves) break;
    }

    if (!hasMoves) {
        if (gen.isKingInCheck(*this, sideToMove)) {
            status = GameStatus::Checkmate;
        } else {
            status = GameStatus::Stalemate;
        }
    }
}