
add_library( toolslib OBJECT "" )

target_sources( toolslib 
	PRIVATE
		ResultHandler.hpp
		TimeFrame.hpp
		TimeFrame.cpp
)

target_compile_features( toolslib PUBLIC cxx_std_20 )