cmake_minimum_required(VERSION 3.22)
project(untitled1)

set(CMAKE_CXX_STANDARD 23)

set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML COMPONENTS graphics window system audio)


add_executable(untitled1 main.cpp)
target_link_libraries(untitled1 PUBLIC sfml-graphics sfml-window sfml-system sfml-audio sfml-network)

#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/openal32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/htmlimage.png DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/audio DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/material DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
