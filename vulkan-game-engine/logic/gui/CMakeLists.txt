if(NOT POLICY CMP0076)
	set( SUBFOLDER "gui/" )
endif()

target_sources( logiclib 
	PRIVATE
		${SUBFOLDER}Label.cpp
		${SUBFOLDER}Button.cpp
		${SUBFOLDER}ScrollBar.cpp
)

target_sources( logiclib 
	PRIVATE
		${SUBFOLDER}Label.hpp
		${SUBFOLDER}Button.hpp
		${SUBFOLDER}ScrollBar.hpp
)	