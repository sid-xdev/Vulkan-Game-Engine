cmake_minimum_required(  VERSION 3.13.0 )

if( WIN32 )
	add_executable( game WIN32 main.cpp )
endif()

add_subdirectory( math )
add_subdirectory( tools )
add_subdirectory( resources )
add_subdirectory( logic )
add_subdirectory( renderer )
add_subdirectory( shader )

#runtime operation system specifics

add_subdirectory( windows )
target_link_libraries( game windowslib )

target_compile_features( game PUBLIC cxx_std_17 )
target_link_libraries( game ${Vulkan_LIBRARY} 
	resourceslib
	logiclib
	toolslib
	mathlib
	levellib
	renderlib
)

add_custom_target( data )

file( TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resources DATA_SOURCE_PATH )
file( TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/Assets DATA_DESTINATION_PATH )

add_custom_command(
	TARGET data POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${DATA_SOURCE_PATH} ${DATA_DESTINATION_PATH}
	DEPENDS ${DATA_DESTINATION_PATH}
	COMMENT "symbolic link resources folder from ${DATA_SOURCE_PATH} => ${DATA_DESTINATION_PATH}")

add_dependencies( game data )