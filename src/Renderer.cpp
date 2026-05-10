#include "Renderer.hpp"
#include <iostream>

Renderer::Renderer(const char* title, int width, int height) : m_width(width), m_height(height) {
    m_squareSize = std::min(width, height) / 8;

    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
    }
    
    // Ensure logical size matches the window logical size, fixing High DPI scaling issues
    SDL_RenderSetLogicalSize(m_renderer, width, height);
}

Renderer::~Renderer() {
    if (m_boardTexture) SDL_DestroyTexture(m_boardTexture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
}

void Renderer::loadBoardTexture(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface) {
        m_boardTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
        SDL_FreeSurface(surface);
    } else {
        std::cerr << "Failed to load board image: " << IMG_GetError() << std::endl;
    }
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::drawBoard() {
    if (m_boardTexture) {
        // The board.png seems to be 2050x2050 with a 25px border and 250px squares (8 * 250 = 2000).
        // Let's crop out the 25px border so the 8x8 grid fits exactly into the window.
        SDL_Rect srcRect = { 25, 25, 2000, 2000 };
        SDL_Rect destRect = { 0, 0, m_width, m_height };
        SDL_RenderCopy(m_renderer, m_boardTexture, &srcRect, &destRect);
    } else {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                SDL_Rect rect = { c * m_squareSize, r * m_squareSize, m_squareSize, m_squareSize };
                if ((r + c) % 2 == 0) {
                    SDL_SetRenderDrawColor(m_renderer, 240, 217, 181, 255); // Light color
                } else {
                    SDL_SetRenderDrawColor(m_renderer, 181, 136, 99, 255); // Dark color
                }
                SDL_RenderFillRect(m_renderer, &rect);
            }
        }
    }
}

void Renderer::drawPieces(const Board& board, TextureManager& tm) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece({r, c});
            if (p.type != PieceType::None) {
                SDL_Texture* tex = tm.getPieceTexture(p);
                // Make pieces twice as big and center them in the square
                int pieceSize = m_squareSize * 2;
                int offset = (m_squareSize - pieceSize) / 2;
                SDL_Rect rect = { c * m_squareSize + offset, r * m_squareSize + offset, pieceSize, pieceSize };
                
                if (tex) {
                    SDL_RenderCopy(m_renderer, tex, nullptr, &rect);
                } else {
                    // Fallback representation if no texture: Draw simple colored squares
                    if (p.color == Color::White) {
                        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
                    } else {
                        SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
                    }
                    // Make it smaller than the square
                    SDL_Rect inner = { c * m_squareSize + 10, r * m_squareSize + 10, m_squareSize - 20, m_squareSize - 20 };
                    SDL_RenderFillRect(m_renderer, &inner);
                }
            }
        }
    }
}

void Renderer::drawSelection(std::optional<Position> selected) {
    if (selected.has_value()) {
        SDL_Rect rect = { selected->col * m_squareSize, selected->row * m_squareSize, m_squareSize, m_squareSize };
        // Draw highlight (semi-transparent green)
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 100);
        SDL_RenderFillRect(m_renderer, &rect);
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
    }
}

void Renderer::drawLegalMoves(const std::vector<Move>& legalMoves) {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    for (const Move& m : legalMoves) {
        SDL_Rect rect = { m.to.col * m_squareSize, m.to.row * m_squareSize, m_squareSize, m_squareSize };
        
        bool isCapture = (m.type == MoveType::Capture || 
                          m.type == MoveType::PromotionCapture || 
                          m.type == MoveType::EnPassant);
                          
        if (isCapture) {
            // Draw red for captures
            SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 120);
        } else {
            // Draw yellow for normal moves
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 0, 120);
        }
        
        // As a nice touch, draw a slightly smaller rect for non-captures, or just fill the square?
        // Let's draw a circle-like small rect or just a filled highlight
        SDL_Rect innerRect = { rect.x + rect.w/3, rect.y + rect.h/3, rect.w/3, rect.h/3 };
        
        if (isCapture) {
            // Full box for capture
            SDL_RenderFillRect(m_renderer, &rect);
        } else {
            // Small dot for normal move
            SDL_RenderFillRect(m_renderer, &innerRect);
        }
    }
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
}

void Renderer::present() {
    SDL_RenderPresent(m_renderer);
}