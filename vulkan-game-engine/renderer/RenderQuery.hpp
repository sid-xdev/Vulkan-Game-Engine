#pragma once
#include <Defines.hpp>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace noxcain
{
	
	
	class RenderQuery
	{
	public:

		enum class TimeStampIds : UINT32
		{
			START,
			AFTER_GEOMETRY,
			AFTER_GLYPHS,
			AFTER_SHADING,
			BEFOR_POST,
			AFTER_POST,
			BEFOR_OVERLAY,
			AFTER_OVERLAY,
			END
		};
		static constexpr UINT32 TIMESTAMP_COUNT = (UINT32)TimeStampIds::END + 1;


		RenderQuery();
		~RenderQuery();

		vk::QueryPool get_timestamp_pool() const
		{
			return timestamp_pool;
		}

	private:
		vk::QueryPool timestamp_pool;
	};
}