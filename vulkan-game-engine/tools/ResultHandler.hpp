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
		enum class ResultTypes
		{
			INFO,
			WARNING,
			CRITICAL
		} worst_type = ResultTypes::INFO;
		ResultHandler( const ResultHandler& ) = delete;
		ResultHandler& operator= ( const ResultHandler& ) = delete;

		const ResultType success_value;
		std::vector<ResultType> ignore_values;
		std::vector<ResultType> warning_values;

		struct ReceivedErrors
		{
			ResultTypes type = ResultTypes::CRITCIAL;
			ResultType result;
		};
		std::vector<ReceivedErrors> received_errors;

		std::string last_description;

		void check_result( const ResultType& result )
		{
			auto check_values = [result]( const ResultType& value )
			{
				return value == result;
			};

			if( result != success_value )
			{
				if( !std::any_of( ignore_values.begin(), ignore_values.end(), check_values ) )
				{
					ResultTypes result_value_type;
					if( std::any_of( warning_values.begin(), warning_values.end(), check_values ) )
					{
						result_value_type = ResultTypes::WARNING;
						if( worst_type == ResultTypes::INFO )
						{
							worst_type = ResultTypes::WARNING;
						}
					}
					else
					{	
						result_value_type = ResultTypes::CRITICAL;
						worst_type = ResultTypes::CRITICAL;
					}
					received_errors.push_back( { result_value_type, result } );
					//TODO add message in log
				}
			}
		}

	public:
		ResultHandler( const ResultType& default_success_value ) : success_value( default_success_value )
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
			return worst_type == ResultTypes::INFO;
		}

		inline bool is_critical()
		{
			return worst_type == ResultTypes::CRITICAL;
		}

		inline ResultHandler& operator()( const std::string& new_description )
		{
			last_description = new_description;
		}

		void add_warnings( std::initializer_list<ResultType> values )
		{
			warning_values.assign( values );
		}

		void reset()
		{
			worst_type = ResultTypes::INFO;
			received_errors.clear();
		}

		inline ResultType get_last_error()
		{
			if( received_errors.empty() )
			{
				return success_value;
			}
			return received_errors.back().result;
		}
	};
}
