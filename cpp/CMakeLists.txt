cmake_minimum_required(VERSION 3.16)
project(CarrierBridge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIB "Build CarrierBridge as shared library" ON)

# ------------------------------------------------------------------
# third-party: standalone ASIO (header-only)
# Use external/asio if present; otherwise FetchContent
# ------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/external/asio/CMakeLists.txt")
    # User-provided copy in external/
    add_subdirectory(external/asio EXCLUDE_FROM_ALL)
    add_library(asio::asio INTERFACE IMPORTED)
    target_include_directories(asio::asio INTERFACE "${CMAKE_SOURCE_DIR}/external/asio/asio/include")
else()
    include(FetchContent)
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        asio-1-30-2
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(asio)
    add_library(asio::asio INTERFACE IMPORTED)
    target_include_directories(asio::asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
endif()

target_compile_definitions(asio::asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

# ------------------------------------------------------------------
# library: libcarb
# ------------------------------------------------------------------
set(LIBCARB_SOURCES
    src/libcarb/core.cpp
)

if(BUILD_SHARED_LIB)
    add_library(libcarb SHARED ${LIBCARB_SOURCES})
else()
    add_library(libcarb STATIC ${LIBCARB_SOURCES})
endif()

target_include_directories(libcarb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(libcarb PUBLIC asio::asio)
if (MSVC)
    target_compile_definitions(libcarb PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Export library name for pkg config or linking convenience
set_target_properties(libcarb PROPERTIES OUTPUT_NAME carrierbridge)

# ------------------------------------------------------------------
# broker executable
# ------------------------------------------------------------------
add_executable(broker_app src/broker/broker_main.cpp)
target_link_libraries(broker_app PRIVATE libcarb)

# ------------------------------------------------------------------
# client executable
# ------------------------------------------------------------------
add_executable(client_app src/client/client_main.cpp)
target_link_libraries(client_app PRIVATE libcarb)

# ------------------------------------------------------------------
# examples and tests
# ------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

install(TARGETS libcarb broker_app client_app
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

