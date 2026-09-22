cmake_minimum_required(VERSION 3.20)
project(3dmaster_preview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt6 模块
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network)

set(ORIGINAL_3DMASTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3dmaster")
set(OCCT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../occt")

# 引入头文件目录：本项目的 include 以及原始 3dmaster 的 include、OCCT 的 inc
include_directories(
    include
    "${ORIGINAL_3DMASTER_DIR}/include"
    "${OCCT_DIR}/inc"
)

# 源码清单：独立 Preview 源码 + 原始 3dmaster 核心逻辑（只读复用，无任何侵入）
set(SOURCES
    src/preview_main.cpp
    src/preview_window.cpp
    src/ipc_server.cpp
    "${ORIGINAL_3DMASTER_DIR}/src/master_widget.cpp"
    "${ORIGINAL_3DMASTER_DIR}/src/master_sidebar.cpp"
    "${ORIGINAL_3DMASTER_DIR}/src/model_loader_worker.cpp"
)

set(HEADERS
    include/preview_window.h
    include/ipc_server.h
    "${ORIGINAL_3DMASTER_DIR}/include/master_widget.h"
    "${ORIGINAL_3DMASTER_DIR}/include/master_sidebar.h"
    "${ORIGINAL_3DMASTER_DIR}/include/model_loader_worker.h"
    "${ORIGINAL_3DMASTER_DIR}/include/model_data.h"
)

add_executable(3dmaster-preview WIN32 ${SOURCES} ${HEADERS})

target_link_directories(3dmaster-preview PRIVATE "${OCCT_DIR}/win64/vc14/lib")

target_link_libraries(3dmaster-preview PRIVATE
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Network
    TKDESTEP
    TKDEIGES
    TKDEGLTF
    TKDE
    TKRWMesh
    TKXCAF
    TKLCAF
    TKCAF
    TKService
    TKXSBase
    TKMesh
    TKBRep
    TKTopAlgo
    TKG3d
    TKG2d
    TKMath
    TKernel
    TKGeomBase
    TKGeomAlgo
    TKShHealing
)

if(MSVC)
    target_compile_options(3dmaster-preview PRIVATE /utf-8)
    target_compile_definitions(3dmaster-preview PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(WIN32)
    target_link_libraries(3dmaster-preview PRIVATE opengl32 user32)
endif()
