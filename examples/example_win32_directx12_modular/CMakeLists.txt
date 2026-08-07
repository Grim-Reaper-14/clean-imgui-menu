cmake_minimum_required(VERSION 3.24)
project(clean_imgui_d3d12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(clean_imgui_d3d12 WIN32
    src/main.cpp
    src/Core/Application.cpp
    src/Backend/Window/Win32Window.cpp
    src/Backend/Graphics/D3D12/D3D12Backend.cpp
    src/Frontend/Frontend.cpp
    src/Threading_Module/MainThreadDispatcher.cpp
    src/Threading_Module/ThreadPool.cpp
    ../../imgui.cpp
    ../../imgui_draw.cpp
    ../../imgui_tables.cpp
    ../../imgui_widgets.cpp
    ../../backends/imgui_impl_win32.cpp
    ../../backends/imgui_impl_dx12.cpp
)

target_include_directories(clean_imgui_d3d12 PRIVATE
    ../..
    ../../backends
    src
)

target_compile_definitions(clean_imgui_d3d12 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(clean_imgui_d3d12 PRIVATE d3d12 dxgi dxguid)

if(MSVC)
    target_compile_options(clean_imgui_d3d12 PRIVATE /W4 /permissive- /EHsc)
endif()
