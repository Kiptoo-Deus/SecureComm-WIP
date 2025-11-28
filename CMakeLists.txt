cmake_minimum_required(VERSION 3.16)
project(SecureCommPlatform LANGUAGES C CXX)

# Multi-language project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include C++ subproject
add_subdirectory(cpp)

# Note: Go and Rust will have their own build systems
# I'll integrate them later

# Protocol Buffers generation
find_package(Protobuf REQUIRED)

# Generate protos for all languages
set(PROTO_FILES 
    core/proto/securecomm.proto
    core/proto/payment.proto
    core/proto/vpn.proto
)

protobuf_generate_cpp(PROTO_CPP_SRCS PROTO_CPP_HDRS ${PROTO_FILES})

# Create a target for shared protos
add_library(securecomm_protos INTERFACE)
target_include_directories(securecomm_protos INTERFACE 
    ${CMAKE_CURRENT_BINARY_DIR}
)