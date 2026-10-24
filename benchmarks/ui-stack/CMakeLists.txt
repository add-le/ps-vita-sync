cmake_minimum_required(VERSION 3.16)

# UI stack benchmark. Builds one app per stack, for the host (Linux) or the Vita.
#   host: cmake -S . -B build-host && cmake --build build-host
#   vita: ./build-vita.sh   (VitaSDK docker image)
# Engines are fetched by ./fetch.sh into third_party/ (not committed).

if(DEFINED ENV{VITASDK} AND NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
endif()

project(ui_stack_bench C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND DEFINED ENV{VITASDK})
  set(BENCH_VITA ON)
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

set(TP ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(ASSETS_OUT ${CMAKE_CURRENT_BINARY_DIR}/assets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL REQUIRED sdl2 SDL2_ttf freetype2)
if(BENCH_VITA)
  # sdl2.pc links vitashark but omits the libraries it calls into: SceShaccCgExt, which in turn
  # needs taihen (taiInjectDataForUser, taiHookFunctionOffsetForUser, ...).
  set(SDL_LIBS ${SDL_STATIC_LIBRARIES} SceShaccCgExt taihen_stub)
  link_directories(${SDL_STATIC_LIBRARY_DIRS})
else()
  set(SDL_LIBS ${SDL_LIBRARIES})
  link_directories(${SDL_LIBRARY_DIRS})
endif()
# RmlUi's SDL_Renderer backend loads images through SDL_image (stacks A and C only).
pkg_check_modules(SDLIMG REQUIRED SDL2_image)
if(BENCH_VITA)
  set(SDLIMG_LIBS ${SDLIMG_STATIC_LIBRARIES})
else()
  set(SDLIMG_LIBS ${SDLIMG_LIBRARIES})
endif()

# Shared assets: fonts from the repo root and fetch.sh, per-stack documents.
file(MAKE_DIRECTORY ${ASSETS_OUT})
set(SHARED_FONTS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Kanit-Regular.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/../../MaterialSymbols-Regular.ttf
  ${TP}/fonts/NotoSansJP-Regular.ttf)
file(COPY ${SHARED_FONTS} DESTINATION ${ASSETS_OUT})

# bench_app(<target> <title id> <sources> LIBS <libs> ASSETS <files>)
function(bench_app target titleid)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;ASSETS;FONTS" ${ARGN})
  add_executable(${target} common/harness.cpp ${ARG_SOURCES})
  target_include_directories(${target} PRIVATE common ${SDL_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE ${ARG_LIBS} ${SDL_LIBS})
  if(NOT BENCH_VITA)
    target_compile_definitions(${target} PRIVATE ASSETS="${ASSETS_OUT}/")
  endif()
  foreach(a ${ARG_ASSETS})
    configure_file(${a} ${ASSETS_OUT} COPYONLY)
  endforeach()
  if(BENCH_VITA)
    set(VPK_FILES "")
    foreach(a ${ARG_ASSETS} ${ARG_FONTS})
      get_filename_component(n ${a} NAME)
      list(APPEND VPK_FILES FILE ${a} ${n})
    endforeach()
    # Linker map, for the per-library size breakdown (tools/size_by_archive.py).
    target_link_options(${target} PRIVATE "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
    vita_create_self(${target}.self ${target} UNSAFE)
    vita_create_vpk(${target}.vpk ${titleid} ${target}.self
      VERSION 00.01 NAME "UI bench ${target}" ${VPK_FILES})
  endif()
endfunction()

set(OVERLAY_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../Kanit-Regular.ttf)

# 0 — baseline: harness only.
bench_app(bench_empty SYNCB0000 SOURCES empty/main.cpp FONTS ${OVERLAY_FONT})

# A — RmlUi
if(EXISTS ${TP}/RmlUi)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(RMLUI_PRECOMPILED_HEADERS OFF CACHE BOOL "" FORCE)
  set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory(${TP}/RmlUi ${CMAKE_CURRENT_BINARY_DIR}/rmlui EXCLUDE_FROM_ALL)
  set(RMLUI_BACKEND_SRC
    ${TP}/RmlUi/Backends/RmlUi_Renderer_SDL.cpp
    ${TP}/RmlUi/Backends/RmlUi_Platform_SDL.cpp)
  set(RMLUI_ASSETS a_rmlui/explorer.rml a_rmlui/explorer.rcss a_rmlui/explorer-effects.rcss)
  bench_app(bench_a_rmlui SYNCB000A
    SOURCES a_rmlui/main.cpp ${RMLUI_BACKEND_SRC}
    LIBS RmlUi::RmlUi ${SDLIMG_LIBS}
    ASSETS ${RMLUI_ASSETS}
    FONTS ${SHARED_FONTS})
  target_include_directories(bench_a_rmlui PRIVATE ${TP}/RmlUi/Backends)
  # Straight alpha on host too, so host and Vita run the same backend code.
  target_compile_definitions(bench_a_rmlui PRIVATE RMLUI_SDL_VERSION_MAJOR=2 RMLUI_SDL_STRAIGHT_ALPHA)

  # A-gl3: same screen with the GL3 backend and explorer-effects.rcss (shadows, CSS gradients,
  # transforms). On the Vita it runs on vitaGL through SDL's GLES2 renderer, which compiles GLSL
  # at runtime: the console needs ur0:data/libshacccg.suprx (patches/rmlui-gl3-vitagl.patch).
  bench_app(bench_a_rmlui_gl3 SYNCB000G
    SOURCES a_rmlui/main.cpp ${TP}/RmlUi/Backends/RmlUi_Renderer_GL3.cpp ${TP}/RmlUi/Backends/RmlUi_Platform_SDL.cpp
    LIBS RmlUi::RmlUi ${CMAKE_DL_LIBS}
    ASSETS ${RMLUI_ASSETS}
    FONTS ${SHARED_FONTS})
  target_include_directories(bench_a_rmlui_gl3 PRIVATE ${TP}/RmlUi/Backends)
  target_compile_definitions(bench_a_rmlui_gl3 PRIVATE RMLUI_SDL_VERSION_MAJOR=2 A_GL3 A_EFFECTS)
  if(BENCH_VITA)
    target_compile_definitions(bench_a_rmlui_gl3 PRIVATE RMLUI_GL3_CUSTOM_LOADER="vitagl_loader.h"
      BENCH_RENDER_DRIVER="opengles2")
  endif()
endif()

# B — litehtml
if(EXISTS ${TP}/litehtml AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/b_litehtml/main.cpp)
  set(LITEHTML_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(LITEHTML_ENABLE_LINT OFF CACHE BOOL "" FORCE)
  add_subdirectory(${TP}/litehtml ${CMAKE_CURRENT_BINARY_DIR}/litehtml EXCLUDE_FROM_ALL)
  bench_app(bench_b_litehtml SYNCB000B
    SOURCES b_litehtml/main.cpp b_litehtml/container_sdl.cpp
    LIBS litehtml
    ASSETS b_litehtml/explorer.css
    FONTS ${SHARED_FONTS})
endif()

# C — QuickJS + Preact rendering into RmlUi
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/c_quickjs/main.cpp AND EXISTS ${TP}/quickjs AND TARGET RmlUi::RmlUi AND EXISTS ${TP}/js/preact.mjs)
  file(STRINGS ${TP}/quickjs/VERSION QJS_VERSION)
  add_library(quickjs STATIC
    ${TP}/quickjs/quickjs.c ${TP}/quickjs/libregexp.c ${TP}/quickjs/libunicode.c
    ${TP}/quickjs/cutils.c ${TP}/quickjs/dtoa.c)
  target_compile_definitions(quickjs PRIVATE CONFIG_VERSION="${QJS_VERSION}" _GNU_SOURCE)
  target_include_directories(quickjs PUBLIC ${TP}/quickjs)
  if(NOT BENCH_VITA)
    target_link_libraries(quickjs PUBLIC m pthread)
  endif()
  bench_app(bench_c_quickjs SYNCB000C
    SOURCES c_quickjs/main.cpp ${RMLUI_BACKEND_SRC}
    LIBS RmlUi::RmlUi quickjs ${SDLIMG_LIBS}
    ASSETS a_rmlui/explorer.rcss c_quickjs/dom.js c_quickjs/app.js ${TP}/js/preact.mjs ${TP}/js/hooks.mjs ${TP}/js/htm.mjs
    FONTS ${SHARED_FONTS})
  target_include_directories(bench_c_quickjs PRIVATE ${TP}/RmlUi/Backends)
  target_compile_definitions(bench_c_quickjs PRIVATE RMLUI_SDL_VERSION_MAJOR=2 RMLUI_SDL_STRAIGHT_ALPHA)
endif()