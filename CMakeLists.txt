cmake_minimum_required(VERSION 3.14)

project(
    magnet++
    VERSION 0.1
    DESCRIPTION "A torrent client written in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

# external dependencies
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB SOURCES "src/*.cpp" "main.cpp")

add_executable(
    magnet++
    ${SOURCES}
)

find_package(OpenSSL REQUIRED)

target_compile_definitions(magnet++ PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(magnet++ PRIVATE OpenSSL::SSL OpenSSL::Crypto)
