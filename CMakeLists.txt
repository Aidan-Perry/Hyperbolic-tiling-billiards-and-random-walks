cmake_minimum_required(VERSION 3.20)
project(hyperbolic_tiling_billiards VERSION 0.6.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FLINT REQUIRED IMPORTED_TARGET flint)
pkg_check_modules(JANSSON REQUIRED IMPORTED_TARGET jansson)

add_library(htb_core
  src/native/platform.c
  src/native/arithmetic/arithmetic.c
  src/native/arithmetic/rational.c
  src/native/arithmetic/arb_backend.c
  src/native/arithmetic/calcium_backend.c
  src/native/geometry/geometry.c
  src/native/tilings/registry.c
  src/native/tilings/elliptical_square.c
  src/native/tilings/order5_square.c
  src/native/tilings/binary.c
  src/native/billiards/engine.c
  src/native/random_walk/engine.c
  src/native/io/json_io.c
)
target_include_directories(htb_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(htb_core PUBLIC PkgConfig::FLINT PkgConfig::JANSSON)
if(WIN32)
  target_link_libraries(htb_core PUBLIC psapi)
else()
  target_link_libraries(htb_core PUBLIC m)
endif()
target_compile_options(htb_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(htb_engine src/native/main.c)
target_link_libraries(htb_engine PRIVATE htb_core)

enable_testing()
add_executable(htb_native_tests tests/native/test_core.c)
target_link_libraries(htb_native_tests PRIVATE htb_core)
add_test(NAME native_core COMMAND htb_native_tests)

install(TARGETS htb_engine RUNTIME DESTINATION bin)
