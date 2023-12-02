cmake_minimum_required(VERSION 3.5.0)

project(alignment)
set (CMAKE_CXX_STANDARD 17)

# headers from our project
include_directories(include)
# headers from crow (headers-only version)
include_directories(third_party/Crow/include)
# headers from websocketpp (headers-only version)
include_directories(third_party/websocketpp)
# headers from single_include (headers-only version)
include_directories(third_party/json/single_include)


# for apple macos, add the brew's include path into include path
# so that asio and boost headers can be included
IF(APPLE)
include_directories(/opt/homebrew/include)
ENDIF()
# add gflags. gflags requires manually link, and it uses cmake too
add_subdirectory("third_party/gflags")

# all the source files
file(GLOB SOURCES "src/service/*.cpp" "src/controller/*.cpp" "src/model/*.cpp" "src/util/*.cpp")

add_executable(master ${SOURCES} src/master.cpp)
add_executable(slave ${SOURCES} src/slave.cpp)
add_executable(bmaster ${SOURCES} src/bmaster.cpp)

target_link_libraries(master gflags::gflags)
target_link_libraries(slave gflags::gflags)
target_link_libraries(bmaster gflags::gflags)


