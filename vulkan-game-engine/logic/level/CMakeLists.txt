cmake_minimum_required( VERSION 3.13.0 )

add_library( levellib OBJECT "" )

target_sources( levellib 
	PRIVATE
		HexField.cpp
		MineSweeperLevel.cpp
)

target_sources( levellib 
	PRIVATE
		HexField.hpp
		MineSweeperLevel.hpp
)	

target_compile_features( levellib PUBLIC cxx_std_17 )