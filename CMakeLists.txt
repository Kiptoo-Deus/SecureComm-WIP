cmake_minimum_required(VERSION 3.16)
project(CarrierBridge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------
# Fetch standalone ASIO (header-only)
# ------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2           # stable release (Oct 2024) – avoid "master"
    GIT_SHALLOW    TRUE                  # faster clone
    EXCLUDE_FROM_ALL                     # don't add ASIO's own examples/tests to my build
)

# This will download ASIO the first time I configure
FetchContent_MakeAvailable(asio)

# Create a nice modern interface target
add_library(asio::asio INTERFACE IMPORTED GLOBAL)
target_include_directories(asio::asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
target_compile_definitions(asio::asio INTERFACE
    ASIO_STANDALONE
    ASIO_NO_DEPRECATED
    ASIO_HAS_STD_CHRONO
    ASIO_HAS_MOVE
)

# Optional C++20 coroutines support (highly recommended)
if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    target_compile_definitions(asio::asio INTERFACE
        ASIO_HAS_CO_AWAIT
        ASIO_HAS_STD_COROUTINE
    )
endif()

# ------------------------------------------------------------------
# My library
# ------------------------------------------------------------------
add_library(carrierbridge src/carrierbridge/core.cpp)

target_include_directories(carrierbridge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(carrierbridge PUBLIC asio::asio)

# ------------------------------------------------------------------
# CLI executable
# ------------------------------------------------------------------
add_executable(cb_cli src/app/main.cpp)
target_link_libraries(cb_cli PRIVATE carrierbridge)


set_target_properties(cb_cli PROPERTIES OUTPUT_NAME carrierbridgectl)

# ------------------------------------------------------------------
# Install rules 
# ------------------------------------------------------------------
install(TARGETS carrierbridge cb_cli
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)