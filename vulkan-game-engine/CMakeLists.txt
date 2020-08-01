
set( GAME_SOURCE
	PresentationSurface.hpp
	ResourceFile.hpp
	Defines.hpp
	$<TARGET_OBJECTS:resourceslib> 
	$<TARGET_OBJECTS:logiclib>
	$<TARGET_OBJECTS:toolslib>
	$<TARGET_OBJECTS:mathlib>
	$<TARGET_OBJECTS:levellib>
	$<TARGET_OBJECTS:renderlib>
	$<TARGET_OBJECTS:windowslib>
	$<TARGET_OBJECTS:androidlib>)

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) 
	add_executable( game WIN32 main.cpp ${GAME_SOURCE} )
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
	message( "ANDROID" )
	add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
	find_library( log-lib log )

	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

	add_library( game SHARED main.cpp ${GAME_SOURCE} )
endif()

add_subdirectory( math )
add_subdirectory( tools )
add_subdirectory( resources )
add_subdirectory( logic )
add_subdirectory( renderer )
add_subdirectory( shader )
add_subdirectory( windows )
add_subdirectory( android )

target_compile_features( game PUBLIC cxx_std_20 )
target_link_libraries( game ${Vulkan_LIBRARY} )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) 
	set_target_properties( game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources" )
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
	target_link_libraries( game
		android
		native_app_glue
		${log-lib} )
endif()