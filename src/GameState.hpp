#pragma once
#include "Types.hpp"
#include "Board.hpp"
#include <optional>

enum class GameStatus { Active, Checkmate, Stalemate };

class GameState {
public:
    Board board;

    Color sideToMove = Color::White;
    GameStatus status = GameStatus::Active;
    CastlingRights castlingRights;
    std::optional<Position> enPassantTarget;

    int halfmoveClock = 0;
    int fullmoveNumber = 1;

    std::optional<Position> selectedSquare;

    void selectSquare(Position pos);
    bool tryMove(Position to);
    
    // Core move mechanics
    void makeMoveWithoutValidation(const Move& move);
    bool makeMove(const Move& move); // Validates through generator implicitly

    void updateStatus(); // Checks for checkmate and stalemate

private:
    void applyEnPassant(const Move& move);
    void applyCastling(const Move& move);
    void applyPromotion(const Move& move);
    void updateCastlingRights(const Move& move, Piece movingPiece);
    void updateEnPassantTarget(const Move& move);
    void switchTurn();
};