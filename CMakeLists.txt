cmake_minimum_required(VERSION 3.2)

project(tsck-sensory-substitution C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -g")

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME} src/b64/base64.c src/mg/mongoose.c src/smmServer.cpp src/main.cpp)

target_link_libraries(${PROJECT_NAME} portaudio ssl crypto Threads::Threads ${OpenCV_LIBS})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
endif()       

#add_custom_command(
#  TARGET ${PROJECT_NAME} POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy
#  ${CMAKE_SOURCE_DIR}/settings.yaml
#  ${CMAKE_CURRENT_BINARY_DIR}/settings.yaml)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/web_root
  ${CMAKE_CURRENT_BINARY_DIR}/web_root)


