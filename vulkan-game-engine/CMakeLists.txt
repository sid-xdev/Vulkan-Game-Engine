
set( GAME_SOURCE
	PresentationSurface.hpp
	Defines.hpp
	$<TARGET_OBJECTS:resourceslib> 
	$<TARGET_OBJECTS:logiclib>
	$<TARGET_OBJECTS:toolslib>
	$<TARGET_OBJECTS:mathlib>
	$<TARGET_OBJECTS:levellib>
	$<TARGET_OBJECTS:renderlib> )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) 
	set( GAME_SOURCE ${GAME_SOURCE} $<TARGET_OBJECTS:windowslib> )
	add_executable( game WIN32 main.cpp ${GAME_SOURCE} )
	add_subdirectory( windows )
endif()

add_custom_target(genexdebug COMMAND ${CMAKE_COMMAND} -E echo "$<PLATFORM_ID:Android>")

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
	message( "ANDROID" )
	add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
	find_library( log-lib log )

	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
	
	set( GAME_SOURCE ${GAME_SOURCE} $<TARGET_OBJECTS:androidlib> )
	add_library( game SHARED main.cpp ${GAME_SOURCE} )
	
	add_subdirectory( android )
endif()



add_subdirectory( math )
add_subdirectory( tools )
add_subdirectory( resources )
add_subdirectory( logic )
add_subdirectory( renderer )
add_subdirectory( shader )

target_compile_features( game PUBLIC cxx_std_17 )
target_link_libraries( game ${Vulkan_LIBRARY} )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) 

	add_custom_target( assets )

	file( TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resources DATA_SOURCE_PATH )
	file( TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Assets DATA_DESTINATION_PATH )

	add_custom_command(
		TARGET assets POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${DATA_SOURCE_PATH} ${DATA_DESTINATION_PATH}
		DEPENDS ${DATA_DESTINATION_PATH}
		COMMENT "symbolic link resources folder from ${DATA_SOURCE_PATH} => ${DATA_DESTINATION_PATH}")

	add_dependencies( game assets )

endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
	target_link_libraries( game
		android
		native_app_glue
		${log-lib} )
endif()