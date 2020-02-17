cmake_minimum_required(VERSION 3.2)

project(tsck-sensory-substitution C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(tsck-sensory-substitution src/b64/base64.c src/mg/mongoose.c src/smmServer.cpp src/util.cpp src/getCallbacks.cpp src/postCallbacks.cpp src/main.cpp)

target_link_libraries(tsck-sensory-substitution ssl crypto Threads::Threads ${OpenCV_LIBS})

if(WIN32)
  target_link_libraries(tsck-sensory-substitution ws2_32)
endif()       
