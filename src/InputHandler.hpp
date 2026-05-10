#pragma once
#include <SDL2/SDL.h>

class InputHandler {
public:
    void update();
    bool quitRequested() const { return m_quit; }
    
    bool isMouseButtonPressed(int& x, int& y) const { 
        if (m_mouseClicked) {
            x = m_mouseX;
            y = m_mouseY;
            return true;
        }
        return false;
    }

private:
    bool m_quit = false;
    bool m_mouseClicked = false;
    int m_mouseX = 0;
    int m_mouseY = 0;
};