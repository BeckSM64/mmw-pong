cmake_minimum_required(VERSION 3.15)
project(ChatApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Force static libraries for all subprojects
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

# Fetch your middleware from Git
FetchContent_Declare(
  mmw
  GIT_REPOSITORY https://github.com/BeckSM64/Minimal-Middleware.git
  GIT_TAG revert-cast-change
)
FetchContent_MakeAvailable(mmw)
set(BUILD_BROKER ON CACHE BOOL "Build broker executable")

# Fetch SFML (unchanged)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.5.1
)
FetchContent_MakeAvailable(SFML)

# Add executables
add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

# Link against middleware and SFML
target_link_libraries(Server PRIVATE mmw::mmw pthread sfml-graphics sfml-window sfml-system)
target_link_libraries(Client PRIVATE mmw::mmw pthread sfml-graphics sfml-window sfml-system)

# === Force static linking of GCC runtime + pthread on MinGW ===
if(MINGW)
    # Force static linking of libgcc and libstdc++
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

    # Link static winpthread
    set(WINPTHREAD_STATIC "C:/msys64/mingw64/lib/libwinpthread.a")
    if(EXISTS "${WINPTHREAD_STATIC}")
        target_link_libraries(Server PRIVATE "${WINPTHREAD_STATIC}")
        target_link_libraries(Client PRIVATE "${WINPTHREAD_STATIC}")
    else()
        message(FATAL_ERROR "Static winpthread not found at ${WINPTHREAD_STATIC}")
    endif()
endif()
