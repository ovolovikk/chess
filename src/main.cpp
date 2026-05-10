#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "GameState.hpp"
#include "Renderer.hpp"
#include "TextureManager.hpp"
#include "InputHandler.hpp"

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Renderer renderer("Chess", 800, 800);
    renderer.loadBoardTexture("../assets/board.png");

    TextureManager textureManager(renderer.getSDLRenderer());

    // Load piece textures
    textureManager.loadPieceTexture({PieceType::Pawn, Color::White}, "../assets/chess-pawn-white.png");
    textureManager.loadPieceTexture({PieceType::Pawn, Color::Black}, "../assets/chess-pawn-black.png");
    textureManager.loadPieceTexture({PieceType::Knight, Color::White}, "../assets/chess-knight-white.png");
    textureManager.loadPieceTexture({PieceType::Knight, Color::Black}, "../assets/chess-knight-black.png");
    textureManager.loadPieceTexture({PieceType::Bishop, Color::White}, "../assets/chess-bishop-white.png");
    textureManager.loadPieceTexture({PieceType::Bishop, Color::Black}, "../assets/chess-bishop-black.png");
    textureManager.loadPieceTexture({PieceType::Rook, Color::White}, "../assets/chess-rook-white.png");
    textureManager.loadPieceTexture({PieceType::Rook, Color::Black}, "../assets/chess-rook-black.png");
    textureManager.loadPieceTexture({PieceType::Queen, Color::White}, "../assets/chess-queen-white.png");
    textureManager.loadPieceTexture({PieceType::Queen, Color::Black}, "../assets/chess-queen-black.png");
    textureManager.loadPieceTexture({PieceType::King, Color::White}, "../assets/chess-king-white.png");
    textureManager.loadPieceTexture({PieceType::King, Color::Black}, "../assets/chess-king-black.png");

    InputHandler inputHandler;

    GameState game;

    bool isRunning = true;
    while (isRunning) {
        inputHandler.update();

        if (inputHandler.quitRequested()) {
            isRunning = false;
        }

        int mouseX, mouseY;
        if (inputHandler.isMouseButtonPressed(mouseX, mouseY)) {
            int col = mouseX / renderer.getSquareSize();
            int row = mouseY / renderer.getSquareSize();
            game.selectSquare(Position{row, col});
        }

        renderer.clear();
        renderer.drawBoard();
        renderer.drawSelection(game.selectedSquare);
        renderer.drawPieces(game.board, textureManager);
        renderer.present();

        SDL_Delay(16); // ~60fps
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
