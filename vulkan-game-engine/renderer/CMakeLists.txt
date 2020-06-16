cmake_minimum_required(  VERSION 3.13.0 )

add_library( renderlib OBJECT "" )

target_sources( renderlib 
	PRIVATE
		CommandManager.cpp
		CommandSubmit.cpp
		CommandThreadTools.cpp
		CommandTaskGeometry.cpp
		CommandTaskOverlay.cpp
		CommandTaskSampling.cpp
		CommandTaskVectorDecal.cpp
		DescriptorSetDescription.cpp
		DescriptorSetManager.cpp
		GameGraphicEngine.cpp
		GraphicCore.cpp
		MemoryManagement.cpp
		ShaderManager.cpp
		RenderPassDescription.cpp
		RenderQuery.cpp
)

target_sources( renderlib 
	PRIVATE 
		CommandManager.hpp
		CommandSubmit.hpp
		CommandTasks.hpp
		CommandThreadTools.hpp
		DescriptorSetDescription.hpp
		DescriptorSetManager.hpp
		GameGraphicEngine.hpp
		GraphicCore.hpp
		MemoryManagement.hpp
		GraphicEngineConstants.hpp
		CommandSubpassTask.hpp
		ShaderManager.hpp
		RenderPassDescription.hpp
		RenderQuery.hpp
)	

target_compile_features( renderlib PUBLIC cxx_std_17 )

add_definitions( -DVK_USE_PLATFORM_WIN32_KHR )
add_definitions( -DVULKAN_HPP_NO_EXCEPTIONS )