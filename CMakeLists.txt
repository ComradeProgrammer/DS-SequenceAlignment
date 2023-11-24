cmake_minimum_required(VERSION 3.5.0)

project(alignment)
set (CMAKE_CXX_STANDARD 17)


include_directories(include)
include_directories(third_party/Crow/include)
include_directories(third_party/websocketpp)
include_directories(third_party/json/single_include)


IF(APPLE)
include_directories(/opt/homebrew/include)
ENDIF()

file(GLOB SOURCES "src/service/*.cpp" "src/controller/*.cpp" "src/model/*.cpp" "src/util/*.cpp")

add_executable(master ${SOURCES} src/master.cpp)
add_executable(slave ${SOURCES} src/slave.cpp)
# add_executable(bmaster ${SOURCES} src/backup_master.cpp)



