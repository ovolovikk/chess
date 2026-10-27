#include "InputHandler.hpp"

void InputHandler::update() {
    m_mouseClicked = false; // Reset per frame
    m_aiToggle = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_quit = true;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mouseClicked = true;
                m_mouseX = event.button.x;
                m_mouseY = event.button.y;
            }
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            if (event.key.keysym.sym == SDLK_e) {
                m_aiToggle = true;
            }
        }
    }
}
