cmake_minimum_required(VERSION 3.10)

include(FindPkgConfig)

set(CMAKE_CXX_COMPILER "clang++-11")

project(rogue VERSION 0.1)

pkg_check_modules(glfw   REQUIRED IMPORTED_TARGET "glfw3  >= 3.3")
pkg_check_modules(libpng REQUIRED IMPORTED_TARGET "libpng >= 1.6")
pkg_check_modules(vulkan REQUIRED IMPORTED_TARGET "vulkan >= 1.2")

add_compile_options("-std=c++20")

link_directories(
  "../liberupt/build"
  "../liberupt-ecs/build"
)

add_executable(rogue
  source/main.cc
)

target_include_directories(rogue
  PRIVATE "../liberupt/source"
  PRIVATE "../liberupt-ecs/source"
)

target_link_libraries(rogue
  PkgConfig::glfw
  PkgConfig::libpng
  PkgConfig::vulkan
  ${CMAKE_DL_LIBS}
  "-lerupt"
  "-lerupt-ecs"
  "-lpthread"
)

