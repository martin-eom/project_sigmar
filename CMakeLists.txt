cmake_minimum_required(VERSION 3.21)

project(ProjectSigmar CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

if(MINGW)
    set(_sigmar_saved_lib_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()
find_package(OpenMP REQUIRED)
if(MINGW)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_sigmar_saved_lib_suffixes})
endif()

# Headers shared by both the game and the map editor.
add_library(sigmar_common INTERFACE)
target_include_directories(sigmar_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(sigmar_common INTERFACE _USE_MATH_DEFINES)
target_link_libraries(sigmar_common INTERFACE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    Eigen3::Eigen
    OpenMP::OpenMP_CXX
)

if(MINGW)
    target_link_options(sigmar_common INTERFACE -static-libgcc -static-libstdc++ -static)
endif()

# WIN32 marks these as GUI (non-console) apps on Windows, matching the old
# SubSystem=Windows setting; the code allocates its own debug console via
# AllocConsole(). The keyword has no effect on non-Windows platforms.
add_executable(game WIN32 server.cpp)
target_link_libraries(game PRIVATE sigmar_common)
# The game runs its own OpenMP parallel regions (see physics.h); disabling
# Eigen's internal OpenMP parallelization avoids nested-parallelism overhead.
target_compile_definitions(game PRIVATE EIGEN_DONT_PARALLELIZE)

add_executable(map_editor WIN32 map_editor.cpp)
target_link_libraries(map_editor PRIVATE sigmar_common)

# --- Packaging (CPack) ---
# Layout mirrors the old .vdproj installer: exes + VeraMono.ttf at the
# install root, resource directories alongside, resolved by the code as
# paths relative to the working directory (see textures/, maps/, config/
# usage in view.h/model.h/fileio.h).
install(TARGETS game map_editor RUNTIME DESTINATION .)
install(FILES VeraMono.ttf "Bitstream Vera License.txt" DESTINATION .)
install(DIRECTORY textures/ DESTINATION textures)
install(DIRECTORY config/ DESTINATION config)
# Only the maps checked into git are shipped; the working tree also has
# untracked dev/test maps that aren't part of the release.
install(FILES
    maps/field.json
    maps/maze.json
    maps/pillars.json
    maps/pillars2.json
    maps/testmap.json
    DESTINATION maps
)

set(CPACK_PACKAGE_NAME "Project Sigmar")
set(CPACK_PACKAGE_VENDOR "martin-eom")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Project Sigmar")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "game" "Project Sigmar" "map_editor" "Map Editor")

if(MINGW)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "Project Sigmar")
    set(CPACK_NSIS_DISPLAY_NAME "Project Sigmar")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "game.exe")
    # CPack's NSIS generator looks for a file literally named NSIS.template.in,
    # searching CPACK_MODULE_PATH before its own built-in copy. Ours (vendored
    # from CMake 3.28's stock template) defaults the install directory to
    # wherever the installer .exe is run from and drops admin elevation
    # entirely, matching the old .vdproj's DefaultLocation=[SourceDir] /
    # InstallAllUsers=FALSE.
    set(CPACK_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
