cmake_minimum_required(VERSION 3.24)

project(
    DirectX12GaussianRenderer
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(GaussianRenderer WIN32
    src/Main.cpp
    src/Application.cpp
    src/Application.h
    src/Camera.cpp
    src/Camera.h
    src/D3D12Renderer.cpp
    src/D3D12Renderer.h
    src/Win32Utils.h
)

target_compile_features(GaussianRenderer PRIVATE cxx_std_20)

target_compile_definitions(GaussianRenderer PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(GaussianRenderer PRIVATE
    d3d12
    dxgi
    dxguid
)

if(MSVC)
    target_compile_options(GaussianRenderer PRIVATE
        /W4
        /permissive-
        /EHsc
        /Zc:__cplusplus
    )

    set_property(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT GaussianRenderer
    )

    set_target_properties(GaussianRenderer PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

source_group("Source Files" REGULAR_EXPRESSION "src/.*\\.cpp$")
source_group("Header Files" REGULAR_EXPRESSION "src/.*\\.h$")
