#pragma once
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include <optional>

class GameState {
public:
    Board board;
    Color sideToMove = Color::White;
    std::optional<Position> selectedSquare;

    void selectSquare(Position pos);
    bool tryMove(Position to);
private:
    void toggleSideToMove();
};