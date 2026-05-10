#pragma once
#include "Types.hpp"
#include "GameState.hpp"
#include <vector>

class MoveGenerator {
public:
    std::vector<Move> generatePseudoLegalMoves(const GameState& state, Position from) const;
    std::vector<Move> generateLegalMoves(const GameState& state, Position from) const;

    bool isSquareAttacked(const GameState& state, Position square, Color byColor) const;
    bool isKingInCheck(const GameState& state, Color kingColor) const;

private:
    void generatePawnMoves(const GameState& state, Position from, std::vector<Move>& moves) const;
    void generateKnightMoves(const GameState& state, Position from, std::vector<Move>& moves) const;
    void generateSlidingMoves(const GameState& state, Position from, std::vector<Move>& moves, const std::vector<Position>& directions) const;
    void generateKingMoves(const GameState& state, Position from, std::vector<Move>& moves) const;

    void addPromotionMoves(std::vector<Move>& moves, Position from, Position to, bool isCapture) const;
    
    // Helpers
    bool canMoveTo(const Board& board, Position to, Color ownColor) const;
    bool isEnemyPiece(const Board& board, Position pos, Color ownColor) const;
};