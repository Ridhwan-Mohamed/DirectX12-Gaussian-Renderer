cmake_minimum_required(VERSION 3.24)

project(
    DirectX12GaussianRenderer
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(GaussianSceneCore STATIC
    src/GaussianScene.cpp
    src/GaussianScene.h
    src/PlyGaussianLoader.cpp
    src/PlyGaussianLoader.h
)

target_compile_features(GaussianSceneCore PUBLIC cxx_std_20)
target_include_directories(GaussianSceneCore PUBLIC src)
target_compile_definitions(GaussianSceneCore PUBLIC
    NOMINMAX
    WIN32_LEAN_AND_MEAN
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
    GaussianSceneCore
    d3d12
    dxgi
    dxguid
    shell32
)

if(MSVC)
    target_compile_options(GaussianSceneCore PRIVATE
        /W4
        /permissive-
        /EHsc
        /Zc:__cplusplus
    )

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

add_custom_command(TARGET GaussianRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:GaussianRenderer>/assets"
    COMMENT "Copying Gaussian scene fixtures"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(GaussianLoaderTests
        tests/GaussianLoaderTests.cpp
    )
    target_compile_features(GaussianLoaderTests PRIVATE cxx_std_20)
    target_link_libraries(GaussianLoaderTests PRIVATE GaussianSceneCore)

    if(MSVC)
        target_compile_options(GaussianLoaderTests PRIVATE
            /W4
            /permissive-
            /EHsc
            /Zc:__cplusplus
        )
    endif()

    add_test(
        NAME GaussianLoader.ReferenceFixture
        COMMAND GaussianLoaderTests
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/anchor-splat-demo/point_cloud.ply"
    )
endif()

source_group("Source Files" REGULAR_EXPRESSION "src/.*\\.cpp$")
source_group("Header Files" REGULAR_EXPRESSION "src/.*\\.h$")
