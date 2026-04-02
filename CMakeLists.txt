cmake_minimum_required(VERSION 3.24)
project(WouoUI)

set(CMAKE_C_COMPILER "cc")
set(CMAKE_CXX_COMPILER "c++")
set(CMAKE_NO_SYSTEM_FROM_IMPORTED YES)
set(WOUOUI_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(WOUOUI_SKETCH "../src/main.ino" CACHE STRING "Relative path to the .ino sketch included by Emulator/main.cpp")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(WOUOUI_SDL_LIBS ${SDL2_LIBRARIES})
list(FILTER WOUOUI_SDL_LIBS EXCLUDE REGEX "SDL2main")


file(GLOB SRCS_EMULATOR
        u8g2/csrc/*.c u8g2/sys/sdl/common/*.c u8g2/cppsrc/*.cpp
        Arduino/*.c Arduino/*.cpp
        *.c *.cpp)
include_directories(u8g2/csrc/)
include_directories(u8g2/cppsrc/)
include_directories(Arduino/)
include_directories(Compat/)
include_directories(.)
include_directories(${WOUOUI_ROOT_DIR})
include_directories(${WOUOUI_ROOT_DIR}/include)
include_directories(${WOUOUI_ROOT_DIR}/src)


include_directories(.)
add_executable(WouoUI ${SRCS_EMULATOR})
set_target_properties(WouoUI PROPERTIES OUTPUT_NAME "Emulator")
target_compile_definitions(WouoUI PRIVATE WOUOUI_EMULATOR=1 SDL_MAIN_HANDLED=1 WOUOUI_SKETCH="${WOUOUI_SKETCH}")


target_link_libraries(WouoUI ${WOUOUI_SDL_LIBS} crypto)
if(WIN32 OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    target_link_libraries(WouoUI ws2_32)
endif()

if(WIN32 OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    get_filename_component(WOUOUI_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(WOUOUI_RUNTIME_SEARCH_DIRS
        "/mingw64/bin"
        "${WOUOUI_MINGW_BIN_DIR}"
        "C:/msys64/mingw64/bin")
    foreach(WOUOUI_RUNTIME_DLL SDL2.dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll libcrypto-3-x64.dll)
        unset(WOUOUI_RUNTIME_DLL_PATH CACHE)
        find_file(WOUOUI_RUNTIME_DLL_PATH
            NAMES "${WOUOUI_RUNTIME_DLL}"
            HINTS ${WOUOUI_RUNTIME_SEARCH_DIRS}
            NO_DEFAULT_PATH)
        if(WOUOUI_RUNTIME_DLL_PATH)
            add_custom_command(TARGET WouoUI POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${WOUOUI_RUNTIME_DLL_PATH}"
                    "$<TARGET_FILE_DIR:WouoUI>"
                VERBATIM)
        endif()
    endforeach()
endif()
