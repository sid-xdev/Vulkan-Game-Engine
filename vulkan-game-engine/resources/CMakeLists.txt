cmake_minimum_required(  VERSION 3.13.0 )

add_library( resourceslib OBJECT "" )

target_sources( resourceslib 
	PRIVATE
		GeometryResource.cpp
		GameResourceEngine.cpp
		GameResource.cpp
		FontResource.cpp
		BoundingBox.cpp
		FontEngine.cpp
)

target_sources( resourceslib 
	PRIVATE
		GeometryResource.hpp
		GameResourceEngine.hpp
		GameResource.hpp
		FontResource.hpp
		BoundingBox.hpp
		FontEngine.hpp
)	

target_compile_features( resourceslib PUBLIC cxx_std_17 )