cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME sdl01)
project(${PROJECT_NAME})

set(SOURCE_DIR ${${PROJECT_NAME}_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
list(APPEND LINK_LIBS ${SDL2_LIBRARY})

find_package(SDL2_Image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
list(APPEND LINK_LIBS ${SDL2_IMAGE_LIBRARIES})

set(BIN_DIR "${SOURCE_DIR}/bin")

#### Application ####

add_definitions("-D DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic --std=c++0x")

FILE(GLOB Source_files
	*.cpp
)

FILE(GLOB Header_files
    *.h
)

FILE(GLOB Assets_files
    assets/*.*
)

source_group("Assets" FILES ${Assets_files})

add_executable(${PROJECT_NAME} ${Source_files} ${Assets_files})

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${BIN_DIR})

add_custom_target(postbuild ALL)
#if (WIN32)
    #add_custom_command(TARGET postbuild POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GLEW_DIR}/bin/Release/Win32/glew32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
#endif()

add_custom_command(TARGET postbuild POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${BIN_DIR}/assets)
