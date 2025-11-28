cmake_minimum_required(VERSION 3.16)
project(CarrierBridge LANGUAGES CXX)

# ------------------- ASIO interface -------------------
add_library(asio_interface INTERFACE)
target_include_directories(asio_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/asio/include
)
target_compile_definitions(asio_interface INTERFACE
    ASIO_STANDALONE
    ASIO_NO_DEPRECATED
)
add_library(asio::asio ALIAS asio_interface)

# ------------------- CarrierBridge library -------------------
add_library(carrierbridge_lib
    src/carrierbridge/core.cpp
    include/carrierbridge/carrierbridge.h
    src/carrierbridge/protocol.h
)
target_include_directories(carrierbridge_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(carrierbridge_lib PUBLIC asio::asio)
target_compile_features(carrierbridge_lib PUBLIC cxx_std_20)

# ------------------- C++ CLI client -------------------
add_executable(carrierbridgectl src/app/main.cpp)
target_link_libraries(carrierbridgectl PRIVATE carrierbridge_lib)

# Optional: nicer binary name
set_target_properties(carrierbridgectl PROPERTIES OUTPUT_NAME carrierbridgectl)

message(STATUS "CarrierBridge CMake configured successfully!")
