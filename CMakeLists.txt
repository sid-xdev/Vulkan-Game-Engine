#master cmake file for project vulkan-game-engine

cmake_minimum_required( VERSION 3.13.0 )

string( FIND ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} NX_BUILD_IN_SOURCE )

if( ${NX_BUILD_IN_SOURCE} GREATER -1 )
    message( FATAL_ERROR "No in-source builds! Keep the source clean." )
endif()

project( vulkan-game-engine
	VERSION 1.0
	LANGUAGES CXX )

find_package( Vulkan REQUIRED FATAL_ERROR)

include_directories( vulkan-game-engine )
include_directories( ${Vulkan_INCLUDE_DIR} )
add_subdirectory( vulkan-game-engine )

set( NX_GLSL_SHADER_DIR "${CMAKE_SOURCE_DIR}/build-data/shaders" CACHE PATH "GLSL shader files directory" )
set( NX_GLSL_LANGUAGE_VALIDATOR $ENV{VULKAN_SDK}/bin/glslangValidator CACHE FILEPATH "glslangValidator file path" )


