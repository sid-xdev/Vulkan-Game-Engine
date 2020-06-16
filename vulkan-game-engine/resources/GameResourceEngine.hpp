#pragma once

#include <Defines.hpp>
#include <resources/GameResource.hpp>

#include <mutex>
#include <memory>
#include <vector>
#include <array>

namespace noxcain
{
	class FontResource;
	class GeometryResource;

	class ResourceEngine
	{
	public:
		~ResourceEngine();

		ResourceEngine( const ResourceEngine& ) = delete;
		ResourceEngine( ResourceEngine&& ) = delete;
		ResourceEngine& operator=( const ResourceEngine& ) = delete;
		ResourceEngine& operator=( ResourceEngine&& ) = delete;

		static ResourceEngine& get_engine();

		struct ResourceLimits
		{
			std::size_t glyph_count = 0;
			std::size_t max_size_per_resource = 0;
			std::size_t font_count = 0;
		};

		ResourceLimits get_resource_limits() const
		{
			return resource_limits;
		}

		//BASIC SUBRESOURCE LOGIC
		const std::vector<GameSubResource>& get_subresources()const;
		SubResourceCollection getSubResourcesMetaInfos( GameSubResource::SubResourceType wantedType ) const;

		//SPECIAL FONT BASED LOGIC
		const FontResource& get_font( std::size_t index ) const;
		const std::vector<FontResource>& get_fonts() const
		{
			return font_resources;
		}

		//SPECIAL GEOMTRY CASE
		const GeometryResource& get_geometry( std::size_t index ) const;
		std::size_t get_invalid_geomtry_id() const;
		std::size_t get_invalid_font_id() const;

	private:
		
		ResourceEngine();
		static std::mutex resource_mutex;
		static std::unique_ptr<ResourceEngine> resources;

		std::vector<GameSubResource> subresources;
		
		ResourceLimits resource_limits;

		
		std::vector<FontResource> font_resources;
		std::vector<GeometryResource> indexed_geometry_objects;
		
		void read_hex_geometry();
		void read_font( const std::vector<std::string>& font_path );

		std::size_t addSubResource( std::size_t resource_size, GameSubResource::SubResourceType resource_type );
	};
}