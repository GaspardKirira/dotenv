cmake_minimum_required(VERSION 3.20)
project(dotenv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dotenv INTERFACE)
add_library(dotenv::dotenv ALIAS dotenv)

target_include_directories(dotenv INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if (MSVC)
  target_compile_options(dotenv INTERFACE /W4 /permissive-)
else()
  target_compile_options(dotenv INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
enable_testing()

add_executable(dotenv_basic_test tests/test_basic.cpp)
target_link_libraries(dotenv_basic_test PRIVATE dotenv::dotenv)
add_test(NAME dotenv.basic COMMAND dotenv_basic_test)
