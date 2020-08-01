
add_library( resourceslib OBJECT "" )

target_sources( resourceslib 
	PRIVATE
		GeometryResource.cpp
		GameResourceEngine.cpp
		GameResource.cpp
		FontResource.cpp
		BoundingBox.cpp
		FontEngine.cpp
		ResourceTools.cpp
)

target_sources( resourceslib 
	PRIVATE
		GeometryResource.hpp
		GameResourceEngine.hpp
		GameResource.hpp
		FontResource.hpp
		BoundingBox.hpp
		FontEngine.hpp
		ResourceTools.hpp
)	

target_compile_features( resourceslib PUBLIC cxx_std_20 )