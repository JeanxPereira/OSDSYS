cmake_minimum_required(VERSION 3.15)
project(OSDSYS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
    src/main.cpp
    src/Core.cpp
    src/Renderer.cpp
    src/Assets.cpp
    src/ICOBLoader.cpp
    src/FontLoader.cpp
    src/TextureLoader.cpp
    src/SoundLoader.cpp
    src/VAGDecoder.cpp
    src/scenes/SCELogoScene.cpp
    src/scenes/BootScene.cpp
    src/scenes/MenuScene.cpp
    src/scenes/BrowserScene.cpp
    src/scenes/DebugVu1Scene.cpp
    src/scenes/DebugFontScene.cpp
    src/scenes/DebugSoundScene.cpp
    src/scenes/DebugTextureScene.cpp
)

add_executable(OSDSYS ${SOURCES})

target_include_directories(OSDSYS PRIVATE
    ${OPENGL_INCLUDE_DIR}
    src/
)

target_link_libraries(OSDSYS PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    SDL2_mixer::SDL2_mixer 
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
)

add_custom_command(TARGET OSDSYS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:OSDSYS>/shaders
    COMMENT "Copying shaders to output directory"
)

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET OSDSYS POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:OSDSYS>/assets
        COMMENT "Copying assets to output directory"
    )
endif()