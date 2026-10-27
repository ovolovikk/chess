#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "Assets.hpp"
#include "GameState.hpp"
#include "MoveGenerator.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "TextureManager.hpp"
#include "InputHandler.hpp"

// You ("blue") play White at the bottom and move first. Press E to toggle the
// AI opponent, which plays Black.
constexpr Color HUMAN_COLOR = Color::White;
constexpr Color AI_COLOR = Color::Black;

// Everything one frame needs. The browser can't run a blocking while-loop, so
// the loop body lives in runFrame() and Emscripten calls it once per frame.
struct App {
    Renderer& renderer;
    TextureManager& textureManager;
    InputHandler& inputHandler;
    Engine& engine;
    GameState game;
    bool aiEnabled = false;
    bool isRunning = true;
};

static void runFrame(App& app)
{
    GameState& game = app.game;

    // Game over is checked at the start of the frame, after the final position
    // has already been shown -- in the browser nothing reaches the screen
    // until the frame function returns.
    if (game.status != GameStatus::Active) {
        std::string title = "Game Over";
        std::string msg = "";
        if (game.status == GameStatus::Checkmate) {
            msg = (game.sideToMove == Color::White) ? "Checkmate! Black wins." : "Checkmate! White wins.";
        } else {
            msg = "Stalemate! It's a draw.";
        }

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.c_str(), msg.c_str(), app.renderer.getSDLWindow());

        // Restart the game automatically after dismissing the message box.
        // AI stays in whatever mode it was in.
        game = GameState();
    }

    // The AI thinks at the start of the frame, so the human's last move was
    // already presented by the previous frame while the engine searches.
    if (game.status == GameStatus::Active && app.aiEnabled && game.sideToMove == AI_COLOR) {
        if (auto best = app.engine.findBestMove(game)) {
            game.makeMove(*best);
            game.updateStatus();
        }
    }

    app.inputHandler.update();

    if (app.inputHandler.quitRequested()) {
        app.isRunning = false;
    }

#ifndef __EMSCRIPTEN__
    // The browser build always plays against the engine, so E only works on desktop.
    if (app.inputHandler.aiTogglePressed()) {
        app.aiEnabled = !app.aiEnabled;
        std::cout << "AI " << (app.aiEnabled ? "enabled" : "disabled") << std::endl;
    }
#endif

    // Only let the human move when it is not the AI's turn.
    bool aiToMove = app.aiEnabled && game.sideToMove == AI_COLOR;

    int mouseX, mouseY;
    if (!aiToMove && app.inputHandler.isMouseButtonPressed(mouseX, mouseY)) {
        int col = mouseX / app.renderer.getSquareSize();
        int row = mouseY / app.renderer.getSquareSize();
        if (col >= 0 && col < 8 && row >= 0 && row < 8) {
            game.selectSquare(Position{row, col});
        }
    }

    std::vector<Move> legalMoves;
    if (game.selectedSquare.has_value()) {
        MoveGenerator gen;
        legalMoves = gen.generateLegalMoves(game, *game.selectedSquare);
    }

    app.renderer.clear();
    app.renderer.drawBoard();
    app.renderer.drawSelection(game.selectedSquare);
    app.renderer.drawLegalMoves(legalMoves);
    app.renderer.drawPieces(game.board, app.textureManager);
    app.renderer.drawEvalBar(app.engine.evaluate(game), app.aiEnabled);
    app.renderer.present();
}

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

    const int boardSize = 800;
    const int panelWidth = 90;
    Renderer renderer("Chess", boardSize, panelWidth);
    renderer.loadBoardTexture(ASSET_DIR "board.png");

    TextureManager textureManager(renderer.getSDLRenderer());

    // Load piece textures
    textureManager.loadPieceTexture({PieceType::Pawn, Color::White}, ASSET_DIR "chess-pawn-white.png");
    textureManager.loadPieceTexture({PieceType::Pawn, Color::Black}, ASSET_DIR "chess-pawn-black.png");
    textureManager.loadPieceTexture({PieceType::Knight, Color::White}, ASSET_DIR "chess-knight-white.png");
    textureManager.loadPieceTexture({PieceType::Knight, Color::Black}, ASSET_DIR "chess-knight-black.png");
    textureManager.loadPieceTexture({PieceType::Bishop, Color::White}, ASSET_DIR "chess-bishop-white.png");
    textureManager.loadPieceTexture({PieceType::Bishop, Color::Black}, ASSET_DIR "chess-bishop-black.png");
    textureManager.loadPieceTexture({PieceType::Rook, Color::White}, ASSET_DIR "chess-rook-white.png");
    textureManager.loadPieceTexture({PieceType::Rook, Color::Black}, ASSET_DIR "chess-rook-black.png");
    textureManager.loadPieceTexture({PieceType::Queen, Color::White}, ASSET_DIR "chess-queen-white.png");
    textureManager.loadPieceTexture({PieceType::Queen, Color::Black}, ASSET_DIR "chess-queen-black.png");
    textureManager.loadPieceTexture({PieceType::King, Color::White}, ASSET_DIR "chess-king-white.png");
    textureManager.loadPieceTexture({PieceType::King, Color::Black}, ASSET_DIR "chess-king-black.png");

    InputHandler inputHandler;
    Engine engine;

    App app{renderer, textureManager, inputHandler, engine};

#ifdef __EMSCRIPTEN__
    // Visitors of the web page come to play the engine, so it is always on,
    // and it thinks for less time because the tab is frozen while it searches.
    app.aiEnabled = true;
    engine.timeLimitMs = 2000;

    // Hands control to the browser; main() never returns, so the stack-allocated
    // objects above stay alive for the lifetime of the page.
    emscripten_set_main_loop_arg([](void* arg) { runFrame(*static_cast<App*>(arg)); }, &app, 0, true);
#else
    while (app.isRunning) {
        runFrame(app);
        SDL_Delay(16); // ~60fps
    }
#endif

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
