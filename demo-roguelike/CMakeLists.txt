cmake_minimum_required(VERSION 3.10)

include(FindPkgConfig)

set(CMAKE_CXX_COMPILER "clang++-11")

project(rogue VERSION 0.1)

pkg_check_modules(glfw   REQUIRED IMPORTED_TARGET "glfw3  >= 3.3")
pkg_check_modules(libpng REQUIRED IMPORTED_TARGET "libpng >= 1.6")
pkg_check_modules(vulkan REQUIRED IMPORTED_TARGET "vulkan >= 1.2")
pkg_check_modules(x11    REQUIRED IMPORTED_TARGET "x11    >= 1.5")

add_compile_options(
  "-std=c++20" 
  "-O3"
)

link_directories("../liberupt/build")

add_executable(rogue
  source/main.cc
)

target_include_directories(rogue
  PRIVATE ".."
)

target_link_libraries(rogue
  PkgConfig::glfw
  PkgConfig::libpng
  PkgConfig::vulkan
  PkgConfig::x11
  ${CMAKE_DL_LIBS}
  "-lerupt"
  "-lpthread"
)

