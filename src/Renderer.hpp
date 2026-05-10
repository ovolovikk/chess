#pragma once
#include <SDL2/SDL.h>
#include "Board.hpp"
#include "GameState.hpp"
#include "TextureManager.hpp"
#include <memory>

class Renderer {
public:
    Renderer(const char* title, int width, int height);
    ~Renderer();

    void loadBoardTexture(const std::string& path);

    void clear();
    void drawBoard();
    void drawPieces(const Board& board, TextureManager& tm);
    void drawSelection(std::optional<Position> selected);
    void present();

    SDL_Renderer* getSDLRenderer() const { return m_renderer; }
    
    int getSquareSize() const { return m_squareSize; }

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    
    int m_width;
    int m_height;
    int m_squareSize;
    
    SDL_Texture* m_boardTexture = nullptr;
};