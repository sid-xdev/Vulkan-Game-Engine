
add_executable( font_engine main.cpp FontEngine.hpp FontEngine.cpp $<TARGET_OBJECTS:resourceslib> )
target_compile_features( font_engine PUBLIC cxx_std_17 )