cmake_minimum_required(VERSION 3.9)
project(DecisionTree VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(Boost_INCLUDE_DIR G:/boost_1_75_0/)
set(Boost_INCLUDE_DIRS G:/boost_1_75_0/)
set(Boost_LIBRARIES G:/boost_1_75_0/stage/lib/)
link_directories(G:/boost_1_75_0/stage/lib)


add_subdirectory(lib)
add_subdirectory(test)