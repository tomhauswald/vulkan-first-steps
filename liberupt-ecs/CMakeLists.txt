cmake_minimum_required(VERSION 3.10)

include(FindPkgConfig)

set(CMAKE_CXX_COMPILER "clang++-11")

project(liberupt-ecs VERSION 0.1)

pkg_check_modules("GLFW" "glfw3  >= 3.3")
pkg_check_modules("PNG"  "libpng >= 1.6")

add_compile_options("-std=c++20")

include_directories(../liberupt/source)

add_library(erupt-ecs STATIC
  source/dummy.cc
)
