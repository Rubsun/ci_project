cmake_minimum_required(VERSION 3.15)
project(MemoryTrainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    src/main.cpp
    src/memory_game.cpp
    src/memory_service.cpp
    src/api_controller.cpp
    src/user_service.cpp
    src/card_pairs_game.cpp
)

set(HEADERS
    include/memory_game.h
    include/memory_service.h
    include/api_controller.h
    include/user.h
    include/user_service.h
    include/card_pairs_game.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)

file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR}/bin)

