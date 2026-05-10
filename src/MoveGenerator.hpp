#pragma once
#include "Types.hpp"
#include "Board.hpp"
#include <vector>

class MoveGenerator {
public:
    // Simple verification (allows any piece to capture any other piece not of its own color)
    // For a fully working chess game, implement piece-specific logic here.
    static std::vector<Move> generatePseudoLegalMoves(const Board& board, Position pos);
    static bool isLegalMove(const Board& board, const Move& move, Color sideToMove);
};