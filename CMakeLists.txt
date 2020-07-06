#master cmake file for project vulkan-game-engine

cmake_minimum_required( VERSION 3.13.0 )

string( FIND ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} NX_BUILD_IN_SOURCE )

if( ${NX_BUILD_IN_SOURCE} GREATER -1 )
    message( FATAL_ERROR "No in-source builds! Keep the source clean." )
endif()

project( vulkan-game-engine
	VERSION 1.0
	LANGUAGES C CXX )

find_package( Vulkan REQUIRED FATAL_ERROR)

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) 
	include_directories( ${Vulkan_INCLUDE_DIR} )
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
	include_directories( 
		${ANDROID_NDK}/sources/android/ndk_helper
		${ANDROID_NDK}/sources/third_party/vulkan/src/include
		${ANDROID_NDK}/sources/android/native_app_glue )
endif()

include_directories( vulkan-game-engine )
add_subdirectory( vulkan-game-engine )
add_subdirectory( font-engine )


