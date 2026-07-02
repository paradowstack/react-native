# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.13)
project(css_tests)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

# Set up directories
set(REACT_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

# Include directories
include_directories(
  ${REACT_COMMON_DIR}
  /opt/homebrew/include
)

# Since the CSS library is header-only (no .cpp files), create a mock library with the dependencies
add_library(mock_react_deps INTERFACE)
target_include_directories(mock_react_deps INTERFACE ${REACT_COMMON_DIR})

# Find libraries manually since the React Native build system isn't available
find_library(GLOG_LIBRARIES NAMES glog PATHS /opt/homebrew/lib REQUIRED)

# Add all test source files
file(GLOB CSS_TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Create test executable
add_executable(css_tests ${CSS_TEST_SOURCES} ${CSS_IMPL_SOURCES})

# Set compilation flags to match React Native style
target_compile_options(css_tests PRIVATE -Wpedantic)

# Link libraries
target_link_libraries(css_tests 
  GTest::gtest 
  GTest::gtest_main 
  Threads::Threads
  ${GLOG_LIBRARIES}
  mock_react_deps
)

# Enable testing
enable_testing()
add_test(NAME css_tests COMMAND css_tests)