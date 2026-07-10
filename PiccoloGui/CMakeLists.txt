cmake_minimum_required(VERSION 3.21)
project(PiccoloGui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW for platform windowing and input.
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)

# Dear ImGui core UI library.
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.6
)

# ImPlot for real-time plotting widgets.
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.16
)

FetchContent_MakeAvailable(glfw imgui implot)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

FetchContent_GetProperties(implot)
if(NOT implot_POPULATED)
  FetchContent_Populate(implot)
endif()

add_executable(
  PiccoloGui
  src/main.cpp
  src/connection_panel.cpp
  src/configuration_panel.cpp
  src/plot_panel.cpp
  src/cartesian_panel.cpp
  src/log_panel.cpp
  src/log_utils.cpp
  src/telemetry_panel.cpp
  src/jog_panel.cpp
  src/serial_manager.cpp
  src/serial_port.cpp
  ../AppLayer/Common/Crc16.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)

# Add platform-specific serial port implementation
if(WIN32)
  target_sources(PiccoloGui PRIVATE src/serial_port_windows.cpp)
else()
  target_sources(PiccoloGui PRIVATE src/serial_port_unix.cpp)
endif()

target_include_directories(
  PiccoloGui
  PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)

target_link_libraries(PiccoloGui PRIVATE glfw)

target_compile_definitions(PiccoloGui PRIVATE PICCOLO_GUI)

if(WIN32)
  target_link_libraries(PiccoloGui PRIVATE opengl32)
elseif(UNIX)
  target_link_libraries(PiccoloGui PRIVATE GL)
endif()

if(MSVC)
  target_compile_options(PiccoloGui PRIVATE /W4 /permissive-)
else()
  target_compile_options(PiccoloGui PRIVATE -Wall -Wextra -Wpedantic)
endif()
