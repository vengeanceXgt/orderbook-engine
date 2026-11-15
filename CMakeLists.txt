cmake_minimum_required(VERSION 3.14)
project(quadeye_orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(test_orderbook tests/test_orderbook.cpp src/orderbook.cpp)
add_executable(bench src/bench.cpp src/orderbook.cpp)

if(MSVC)
    target_compile_options(test_orderbook PRIVATE /W4 /O2)
    target_compile_options(bench PRIVATE /W4 /O2)
else()
    target_compile_options(test_orderbook PRIVATE -Wall -Wextra -O2)
    target_compile_options(bench PRIVATE -Wall -Wextra -O3)
endif()

enable_testing()
add_test(NAME OrderBookTests COMMAND test_orderbook)
