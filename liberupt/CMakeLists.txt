cmake_minimum_required(VERSION 3.10)

include(FindPkgConfig)

set(CMAKE_CXX_COMPILER "clang++-11")

project(liberupt VERSION 0.1)

pkg_check_modules("GLFW" "glfw3  >= 3.3")
pkg_check_modules("PNG"  "libpng >= 1.6")

include_directories()

add_compile_options("-std=c++20")
add_library(erupt STATIC
  source/mouse.cc
  source/keyboard.cc
  source/vulkan_context.cc
  source/renderer.cc
)
