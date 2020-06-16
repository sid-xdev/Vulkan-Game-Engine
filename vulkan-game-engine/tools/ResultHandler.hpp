#pragma once
#include <vector>
#include <algorithm>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	template<typename ResultType>
	class ResultHandler
	{
	private:
		ResultHandler( const ResultHandler& ) = delete;
		ResultHandler& operator= ( const ResultHandler& ) = delete;

		const ResultType successValue;
		std::vector<ResultType> ignoreValues;
		std::vector<ResultType> warningValues;

		ResultType last_value;
		bool nothing_wrong = true;

		std::string last_description;

		void check_result( const ResultType& result )
		{
			last_value = result;
			if( last_value != successValue )
			{
				if( !std::any_of( ignoreValues.begin(), ignoreValues.end(), [this]( const ResultType& i ) { return i == last_value; } ) )
				{
					//TODO add message in log
					nothing_wrong = false;
					//DebugBreak();
				}
			}
		}

	public:
		ResultHandler( const ResultType& defaultSuccessValue ) : successValue( defaultSuccessValue ), last_value( defaultSuccessValue )
		{
		}

		void operator<<( const ResultType& result )
		{
			check_result( result );
		}

		template<typename ValueType>
		ValueType operator<<( const std::pair<ResultType, ValueType>& result )
		{
			check_result( std::get<0>( result ) );
			return std::get<1>( result );
		}
		
		template<typename ValueType>
		ValueType operator<<( const vk::ResultValue<ValueType>& result )
		{
			check_result( result.result );
			return result.value;
		}

		template<template <typename> typename MetaType, typename ValueType>
		ValueType operator<<( const MetaType<ValueType>& result )
		{
			check_result( result.result );
			return result.value;
		}

		inline bool all_okay()
		{
			return nothing_wrong;
		}

		inline bool error()
		{
			return !nothing_wrong;
		}

		inline ResultHandler& operator()( const std::string& new_description )
		{
			last_description = new_description;
		}
	};
}
