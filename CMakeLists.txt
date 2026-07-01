cmake_minimum_required(VERSION 3.21)
cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /FS")

project(silicate VERSION 1.0.0)

set(ARC_FEATURE_FULL ON CACHE BOOL "" FORCE)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp

    src/bot/bot.cpp
    src/bot/updater.cpp
    src/bot/scheduler.cpp

    src/assist/autoclicker.cpp
    src/assist/hitboxes.cpp

    src/hooks/CCActionManager.cpp
    src/hooks/CCDirector.cpp
    src/hooks/CCScheduler.cpp
    src/hooks/EnhancedGameObject.cpp
    src/hooks/FMODAudioEngine.cpp
    src/hooks/GJBaseGameLayer.cpp
    src/hooks/GameObject.cpp
    src/hooks/LevelEditorLayer.cpp
    src/hooks/PauseLayer.cpp
    src/hooks/PlayLayer.cpp
    src/hooks/PlayerObject.cpp
    src/hooks/ShaderLayer.cpp
    src/hooks/VideoOptionsLayer.cpp

    src/ui/manager.cpp
    src/ui/hook.cpp
    src/ui/theme.cpp

    src/replay/replay.cpp
    src/replay/system.cpp

    src/physics/collisions.cpp
    src/physics/gjbasegamelayer.cpp
    src/physics/gravity.cpp
    src/physics/object.cpp
    src/physics/player.cpp

    src/label/label.cpp

    src/checkpoint/fix.cpp
    src/checkpoint/checkpoint.cpp

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
    src/trajectory/trajectory.cpp

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()
    src/render/renderer.cpp
    src/render/texture.cpp
    src/render/dsp.cpp
    src/render/pass.cpp

project(libslc)
    src/util/midhook.cpp
    src/hooks/GJEffectManager.cpp
)

add_library(libslc INTERFACE)
target_compile_options(${PROJECT_NAME} PRIVATE /W4)
target_include_directories(${PROJECT_NAME} PRIVATE src include)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
	target_compile_options(libslc INTERFACE -Wall -Wextra -Wpedantic -Werror -Wno-gnu)
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

target_include_directories(libslc INTERFACE include)
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

add_subdirectory(lib/tabby)
set(TABBY_USE_WIN32 ON)
set(TABBY_USE_OPENGL3 ON)
add_compile_definitions(TABBY_USE_WIN32)
add_compile_definitions(TABBY_USE_OPENGL3)
target_link_libraries(${PROJECT_NAME} tabby)

CPMAddPackage("gh:stephenberry/glaze#main")
target_link_libraries(${PROJECT_NAME} glaze::glaze)

CPMAddPackage("gh:zyantific/zydis#bffbb61")
target_link_libraries(${PROJECT_NAME} Zydis)

CPMAddPackage("gh:cursey/safetyhook#main")
target_link_libraries(${PROJECT_NAME} safetyhook)

add_executable(slcconv
  src/slcconv.cpp
)
CPMAddPackage("gh:peonii/slc#main")

target_link_libraries(${PROJECT_NAME} libslc)
target_compile_options(GeodeBindings PRIVATE -Wno-overloaded-virtual -Wno-unused-variable)

target_link_libraries(slcconv PRIVATE libslc)
setup_geode_mod(${PROJECT_NAME})
