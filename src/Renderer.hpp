#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Board.hpp"
#include "GameState.hpp"
#include "TextureManager.hpp"
#include <memory>
#include <vector>

class Renderer {
public:
    // The board is a square of `boardSize` pixels on the left; `panelWidth`
    // extra pixels on the right hold the evaluation bar.
    Renderer(const char* title, int boardSize, int panelWidth);
    ~Renderer();

    void loadBoardTexture(const std::string& path);

    void clear();
    void drawBoard();
    void drawPieces(const Board& board, TextureManager& tm);
    void drawSelection(std::optional<Position> selected);
    void drawLegalMoves(const std::vector<Move>& legalMoves);

    // Draws the right-hand advantage bar. `evalWhiteCp` is the engine's
    // evaluation in centipawns from White's perspective (positive = White
    // ahead). The player (White) is shown in blue filling from the bottom;
    // the opponent (Black) is shown in white filling from the top.
    void drawEvalBar(int evalWhiteCp, bool aiEnabled);

    void present();

    SDL_Renderer* getSDLRenderer() const { return m_renderer; }
    SDL_Window* getSDLWindow() const { return m_window; }

    int getSquareSize() const { return m_squareSize; }
    int getBoardSize() const { return m_boardSize; }

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    int m_boardSize;
    int m_panelWidth;
    int m_width;
    int m_height;
    int m_squareSize;

    SDL_Texture* m_boardTexture = nullptr;

    TTF_Font* m_font = nullptr; // May be null if no system font was found.
    void drawText(const std::string& text, int x, int y, SDL_Color color);
};
