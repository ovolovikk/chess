#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>
#include "Types.hpp"

class TextureManager {
public:
    TextureManager(SDL_Renderer* renderer);
    ~TextureManager();

    // Since we don't have assets yet, we'll draw simple rects or load basic sprites if provided
    void loadPieceTexture(Piece piece, const std::string& path);
    SDL_Texture* getPieceTexture(Piece piece);

private:
    SDL_Renderer* m_renderer;
    // Map based on piece type and color (e.g., string key like "White_Pawn")
    std::unordered_map<std::string, SDL_Texture*> m_textures;
    
    std::string getKey(Piece piece) const;
};