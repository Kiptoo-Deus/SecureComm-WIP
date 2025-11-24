cmake_minimum_required(VERSION 3.20)
project(CarrierBridge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)

FetchContent_MakeAvailable(asio)

add_library(carrierbridge SHARED
    src/carrierbridge/core.cpp
)

target_include_directories(carrierbridge PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/carrierbridge/include>
)

target_link_libraries(carrierbridge PRIVATE asio)
target_compile_definitions(carrierbridge PRIVATE ASIO_STANDALONE)

# CLI app
add_executable(carrierbridgectl src/app/main.cpp)
target_link_libraries(carrierbridgectl PRIVATE carrierbridge)

enable_testing()
add_subdirectory(tests)
