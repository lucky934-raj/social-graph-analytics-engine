cmake_minimum_required(VERSION 3.14)
project(SocialGraphAnalyticsEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators (Makefiles, Ninja) build without optimisation unless
# a build type is given, which would make the benchmark meaningless.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# All graph code lives in one library so the CLI and the tests use exactly the
# same compiled code.
add_library(graph_core STATIC
    src/SocialGraph.cpp
    src/GraphAlgorithms.cpp
    src/RecommendationEngine.cpp
    src/DisjointSetUnion.cpp
    src/Connectivity.cpp
    src/GraphStorage.cpp
    src/TextParsing.cpp
    src/CommandProcessor.cpp
)
target_include_directories(graph_core PUBLIC include)

add_executable(social_graph src/main.cpp)
target_link_libraries(social_graph PRIVATE graph_core)

# Not part of the test suite; run it manually (see README).
add_executable(bfs_benchmark bench/bfs_benchmark.cpp)
target_link_libraries(bfs_benchmark PRIVATE graph_core)

enable_testing()
add_executable(unit_tests
    tests/test_main.cpp
    tests/test_social_graph.cpp
    tests/test_algorithms.cpp
    tests/test_recommendations.cpp
    tests/test_connectivity.cpp
    tests/test_storage.cpp
    tests/test_edge_cases.cpp
    tests/test_cli.cpp
)
target_link_libraries(unit_tests PRIVATE graph_core)
# Lets the tests check that the shipped sample data still loads.
target_compile_definitions(unit_tests PRIVATE
    SAMPLE_GRAPH_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/sample_graph.txt")
add_test(NAME unit_tests COMMAND unit_tests)
# A bug such as a missing visited check makes a search loop forever; fail
# instead of hanging.
set_tests_properties(unit_tests PROPERTIES TIMEOUT 60)
