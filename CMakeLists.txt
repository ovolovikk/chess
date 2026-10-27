cmake_minimum_required(VERSION 3.26)
project(chess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to an optimized build so the engine search stays fast.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/Board.cpp
    src/Engine.cpp
    src/GameState.cpp
    src/InputHandler.cpp
    src/MoveGenerator.cpp
    src/Renderer.cpp
    src/TextureManager.cpp
)

if(EMSCRIPTEN)
    # Web build: SDL2, SDL2_image and SDL2_ttf come from Emscripten's ports,
    # and the assets folder is packed into index.data, mounted at /assets.
    set(SDL_PORTS "SHELL:-sUSE_SDL=2" "SHELL:-sUSE_SDL_IMAGE=2" "SHELL:-sSDL2_IMAGE_FORMATS=[\"png\"]" "SHELL:-sUSE_SDL_TTF=2")
    target_compile_options(${PROJECT_NAME} PRIVATE ${SDL_PORTS})
    target_link_options(${PROJECT_NAME} PRIVATE ${SDL_PORTS}
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS})
endif()
