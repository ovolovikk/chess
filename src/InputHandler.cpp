#include "InputHandler.hpp"

void InputHandler::update() {
    m_mouseClicked = false; // Reset per frame

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
    }
}