cmake_minimum_required(VERSION 3.10)
project(carrierbridge_jni_stub)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    message(STATUS "Building stub carrierbridge_jni for Android ABI ${ANDROID_ABI}")
endif()

# Build a small JNI stub that provides the native symbols expected by the Kotlin layer.
# This avoids pulling the full securecomm native tree and its external dependencies
# during early UI/dev testing. Replace with the real native build once native
# dependencies are available and JNI signatures are reconciled.

add_library(carrierbridge_jni SHARED
    carrierbridge_jni_stub.cpp
)

find_library(log-lib log)

target_include_directories(carrierbridge_jni PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(carrierbridge_jni PRIVATE -fPIC)

if(log-lib)
    target_link_libraries(carrierbridge_jni PRIVATE ${log-lib})
endif()

message(STATUS "Built stub JNI library: carrierbridge_jni")

