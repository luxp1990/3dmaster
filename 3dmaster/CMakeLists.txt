cmake_minimum_required(VERSION 3.20)
project(3dmaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 元对象系统 (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 依赖 Qt6 模块
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# 官方 Seer-sdk 接口定义 (契约层)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Seer-sdk/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Seer-sdk" "${CMAKE_CURRENT_BINARY_DIR}/Seer-sdk")
else()
    include(FetchContent)
    FetchContent_Declare(SeerSdk
        GIT_REPOSITORY https://github.com/ccseer/Seer-sdk.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SeerSdk)
endif()

include_directories(include)

set(SOURCES
    src/plugin_entry.cpp
    src/master_viewer.cpp
    src/master_widget.cpp
    src/master_sidebar.cpp
    src/model_loader_worker.cpp
)

set(HEADERS
    include/model_data.h
    include/master_viewer.h
    include/master_widget.h
    include/master_sidebar.h
    include/model_loader_worker.h
    "${SeerSdk_SOURCE_DIR}/seer/viewerbase.h"
)

add_library(3dmaster SHARED ${SOURCES} ${HEADERS})

target_include_directories(3dmaster PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/inc")
target_link_directories(3dmaster PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/win64/vc14/lib")

target_link_libraries(3dmaster PRIVATE
    SeerSdk::SeerSdk
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
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
    target_compile_options(3dmaster PRIVATE /utf-8)
endif()

if(WIN32)
    target_link_libraries(3dmaster PRIVATE opengl32)
endif()

# 输出重命名为 3dmaster.dll
set_target_properties(3dmaster PROPERTIES
    OUTPUT_NAME "3dmaster"
    PREFIX ""
)

# 自动复制 plugin.json 与 3dmaster.ini 到输出目录，并同步 3dmaster.dll 到根目录
add_custom_command(TARGET 3dmaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
        "$<TARGET_FILE_DIR:3dmaster>/plugin.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/3dmaster.ini"
        "$<TARGET_FILE_DIR:3dmaster>/3dmaster.ini"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:3dmaster>"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dmaster.dll" || ${CMAKE_COMMAND} -E echo "[Notice] 3dmaster.dll locked by active Seer process. Close Seer to update root DLL."
)

# 仿真测试宿主程序 (完全遵循 Seer 官方契约测试)
add_executable(test_host test/test_host.cpp "${SeerSdk_SOURCE_DIR}/seer/viewerbase.h")
target_link_libraries(test_host PRIVATE
    3dmaster
    SeerSdk::SeerSdk
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

# 验证 OCCT STEP 离散化解析独立执行程序
add_executable(test_occt_step test/test_occt_step.cpp)
target_include_directories(test_occt_step PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/inc")
target_link_directories(test_occt_step PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/win64/vc14/lib")
target_link_libraries(test_occt_step PRIVATE
    TKDESTEP
    TKXCAF
    TKLCAF
    TKCAF
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

add_executable(test_gltf test/test_gltf.cpp)
target_include_directories(test_gltf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/inc")
target_link_directories(test_gltf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../occt/win64/vc14/lib")
target_link_libraries(test_gltf PRIVATE
    TKDEGLTF
    TKDE
    TKRWMesh
    TKXCAF
    TKLCAF
    TKBRep
    TKTopAlgo
    TKG3d
    TKMath
    TKernel
    TKService
)

add_executable(test_3mf test/test_3mf.cpp)
target_link_libraries(test_3mf PRIVATE
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
)



