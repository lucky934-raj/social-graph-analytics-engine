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
    src/CommandProcessor.cpp
)
target_include_directories(graph_core PUBLIC include)

add_executable(social_graph src/main.cpp)
target_link_libraries(social_graph PRIVATE graph_core)

enable_testing()
add_executable(unit_tests
    tests/test_main.cpp
    tests/test_social_graph.cpp
    tests/test_algorithms.cpp
    tests/test_recommendations.cpp
    tests/test_cli.cpp
)
target_link_libraries(unit_tests PRIVATE graph_core)
add_test(NAME unit_tests COMMAND unit_tests)
