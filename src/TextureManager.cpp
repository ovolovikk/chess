#include "TextureManager.hpp"
#include <iostream>

TextureManager::TextureManager(SDL_Renderer* renderer) : m_renderer(renderer) {}

TextureManager::~TextureManager() {
    for (auto& pair : m_textures) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    m_textures.clear();
}

std::string TextureManager::getKey(Piece piece) const {
    std::string key = (piece.color == Color::White) ? "W_" : "B_";
    switch (piece.type) {
        case PieceType::Pawn: key += "Pawn"; break;
        case PieceType::Knight: key += "Knight"; break;
        case PieceType::Bishop: key += "Bishop"; break;
        case PieceType::Rook: key += "Rook"; break;
        case PieceType::Queen: key += "Queen"; break;
        case PieceType::King: key += "King"; break;
        default: key += "None"; break;
    }
    return key;
}

void TextureManager::loadPieceTexture(Piece piece, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image " << path << ": " << IMG_GetError() << std::endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    
    if (texture) {
        m_textures[getKey(piece)] = texture;
    }
}

SDL_Texture* TextureManager::getPieceTexture(Piece piece) {
    std::string key = getKey(piece);
    if (m_textures.find(key) != m_textures.end()) {
        return m_textures[key];
    }
    return nullptr;
}