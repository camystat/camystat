cmake_minimum_required(VERSION 3.20)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Match the MSVC runtime of the parent project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(_camystat_src "${CMAKE_SOURCE_DIR}/camystat")

add_executable(camystat_tests
  SavgolTests.cpp
  SmoothingTests.cpp
  "${_camystat_src}/Camystat.cpp"
  "${_camystat_src}/Savgol.cpp"
)

target_include_directories(camystat_tests
  PRIVATE
    "${_camystat_src}"
    "${CMAKE_SOURCE_DIR}/eigen"
)

find_package(wxWidgets REQUIRED COMPONENTS core base)
target_link_libraries(camystat_tests PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(camystat_tests PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(camystat_tests PRIVATE ${wxWidgets_DEFINITIONS})

find_package(OpenCV REQUIRED)
target_link_libraries(camystat_tests PRIVATE ${OpenCV_LIBS})
target_include_directories(camystat_tests PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(camystat_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
# Generous timeout: the first launch of a freshly linked binary can be slow (e.g. macOS security scan)
gtest_discover_tests(camystat_tests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 120)
