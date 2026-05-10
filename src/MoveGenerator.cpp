#include "MoveGenerator.hpp"
#include <cmath>

std::vector<Move> MoveGenerator::generatePseudoLegalMoves(const Board& board, Position pos) {
    std::vector<Move> moves;
    Piece p = board.getPiece(pos);
    if (p.type == PieceType::None) return moves;

    // Really naive move generation just to make it playable / testing.
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Position to{r, c};
            if (to == pos) continue;
            
            // Allow capture of opposite color or moving to empty
            Piece dest = board.getPiece(to);
            if (dest.color != p.color) {
                moves.push_back({pos, to});
            }
        }
    }
    return moves;
}

bool MoveGenerator::isLegalMove(const Board& board, const Move& move, Color sideToMove) {
    Piece p = board.getPiece(move.from);
    if (p.color != sideToMove) return false;
    
    // Very basic check: target square is empty or enemy piece
    Piece dest = board.getPiece(move.to);
    if (dest.color == sideToMove) return false;
    
    return true; // Assume true for testability in this step
}