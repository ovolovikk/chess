#include "Renderer.hpp"
#include "Assets.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

Renderer::Renderer(const char* title, int boardSize, int panelWidth)
    : m_boardSize(boardSize), m_panelWidth(panelWidth) {
    m_width = boardSize + panelWidth;
    m_height = boardSize;
    m_squareSize = boardSize / 8;

    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
    }

    // Ensure logical size matches the window logical size, fixing High DPI scaling issues
    SDL_RenderSetLogicalSize(m_renderer, m_width, m_height);

    // Try the bundled font first (the only one available in the browser), then
    // a few common macOS system fonts. If none load, the bar still renders
    // fine, just without the numeric score.
    const char* fontCandidates[] = {
        ASSET_DIR "fonts/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/Library/Fonts/Arial.ttf",
    };
    for (const char* path : fontCandidates) {
        m_font = TTF_OpenFont(path, 18);
        if (m_font) break;
    }
}

Renderer::~Renderer() {
    if (m_font) TTF_CloseFont(m_font);
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
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::drawBoard() {
    if (m_boardTexture) {
        // The board.png seems to be 2050x2050 with a 25px border and 250px squares (8 * 250 = 2000).
        // Let's crop out the 25px border so the 8x8 grid fits exactly into the board area.
        SDL_Rect srcRect = { 25, 25, 2000, 2000 };
        SDL_Rect destRect = { 0, 0, m_boardSize, m_boardSize };
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

        SDL_Rect innerRect = { rect.x + rect.w/3, rect.y + rect.h/3, rect.w/3, rect.h/3 };

        if (isCapture) {
            SDL_RenderFillRect(m_renderer, &rect);
        } else {
            SDL_RenderFillRect(m_renderer, &innerRect);
        }
    }
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!m_font || text.empty()) return;
    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);
    if (tex) {
        SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
}

void Renderer::drawEvalBar(int evalWhiteCp, bool aiEnabled) {
    const int margin = 12;
    const int barX = m_boardSize + margin;
    const int barW = m_panelWidth - 2 * margin;
    const int labelH = 26; // reserve space top & bottom for the score labels
    const int barY = labelH;
    const int barH = m_boardSize - 2 * labelH;

    // Map the centipawn score to White's share of the bar with a logistic
    // curve, so small material edges move the bar noticeably while huge edges
    // saturate rather than overflow. Divisor ~400cp ≈ "one piece tilts it hard".
    double whiteShare = 1.0 / (1.0 + std::exp(-evalWhiteCp / 400.0));
    whiteShare = std::clamp(whiteShare, 0.02, 0.98);

    // Player (White) is blue and fills from the bottom; opponent (Black) is
    // white and fills from the top.
    int blueH = static_cast<int>(barH * whiteShare);
    int whiteH = barH - blueH;

    SDL_Rect whiteRect = { barX, barY, barW, whiteH };
    SDL_SetRenderDrawColor(m_renderer, 235, 235, 235, 255);
    SDL_RenderFillRect(m_renderer, &whiteRect);

    SDL_Rect blueRect = { barX, barY + whiteH, barW, blueH };
    SDL_SetRenderDrawColor(m_renderer, 50, 110, 220, 255);
    SDL_RenderFillRect(m_renderer, &blueRect);

    // Midline marker (even position).
    SDL_SetRenderDrawColor(m_renderer, 120, 120, 120, 255);
    SDL_Rect mid = { barX, barY + barH / 2 - 1, barW, 2 };
    SDL_RenderFillRect(m_renderer, &mid);

    // Border around the bar.
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_Rect border = { barX, barY, barW, barH };
    SDL_RenderDrawRect(m_renderer, &border);

    // Numeric score in pawns, from the player's (White's) perspective.
    double pawns = evalWhiteCp / 100.0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%+.1f", pawns);

    SDL_Color black = { 20, 20, 20, 255 };
    SDL_Color blue = { 50, 110, 220, 255 };
    // Whoever is ahead gets their score shown on their end of the bar.
    if (evalWhiteCp >= 0) {
        drawText(buf, barX, m_boardSize - labelH + 2, blue);   // bottom = player
    } else {
        drawText(buf, barX, 4, black);                         // top = opponent
    }

    // Show whether the AI is active, on a dark badge so it reads against both
    // the white and the blue half of the bar.
    if (m_font) {
        const char* label = aiEnabled ? "AI: ON" : "AI: OFF";
        int textW = 0, textH = 0;
        TTF_SizeText(m_font, label, &textW, &textH);
        const int pad = 4;
        SDL_Rect badge = { barX + (barW - textW) / 2 - pad, m_boardSize / 2 - textH / 2 - pad,
                           textW + 2 * pad, textH + 2 * pad };
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(m_renderer, &badge);
        SDL_Color labelColor = aiEnabled ? SDL_Color{ 120, 220, 120, 255 } : SDL_Color{ 235, 235, 235, 255 };
        drawText(label, badge.x + pad, badge.y + pad, labelColor);
    }
}

void Renderer::present() {
    SDL_RenderPresent(m_renderer);
}
