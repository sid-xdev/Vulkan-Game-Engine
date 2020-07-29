
add_library( logiclib OBJECT "" )

target_sources( logiclib 
	PRIVATE
		GameLogicEngine.cpp
		GeometryLogic.cpp
		InputEventHandler.cpp
		Level.cpp
		Quad2D.cpp
		Region.cpp
		RegionEventReceiver.cpp
		SceneGraph.cpp
		VectorText2D.cpp
		VectorText3D.cpp
		VectorText.cpp
		DebugLevel.cpp
		UserInterface.cpp
)

target_sources( logiclib 
	PRIVATE
		GameLogicEngine.hpp
		GeometryLogic.hpp
		InputEventHandler.hpp
		Level.hpp
		Quad2D.hpp
		Region.hpp
		RegionEventReceiver.hpp
		SceneGraph.hpp
		VectorText2D.hpp
		VectorText3D.hpp
		VectorText.hpp
		DebugLevel.hpp
		Renderable.hpp
		TreeNode.hpp
		UserInterface.hpp
)	

add_subdirectory( level )
add_subdirectory( gui )

target_compile_features( logiclib PUBLIC cxx_std_17 )
