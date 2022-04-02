#ifndef FGL_VULKAN_PIPELINE_HPP_INCLUDE
#define FGL_VULKAN_PIPELINE_HPP_INCLUDE

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <concepts>
#include <cassert>

#include <vulkan/vulkan_raii.hpp>
#include "context.hpp"
#include "memory.hpp"

#include <fgl/utility/zip.hpp>

namespace fgl::vulkan
{

	class Pipeline
	{
		[[nodiscard]] vk::raii::ShaderModule create_shader_module(
			const Context& cntx,
			const std::filesystem::path path
		) const;

		template <std::ranges::forward_range T>
			requires std::same_as<std::ranges::range_value_t<T>, fgl::vulkan::Buffer>
		[[nodiscard]] vk::raii::DescriptorSetLayout create_descriptor_set_layout(
			const Context& cntx,
			const T& buffers )
		{
			std::vector<vk::DescriptorSetLayoutBinding> setBindings;
			for( const auto& buffer : buffers )
			{
				constexpr uint32_t descriptor_count { 1 };
				setBindings.emplace_back(
					buffer.binding,
					buffer.buffer_type,
					descriptor_count,
					vk::ShaderStageFlagBits::eCompute
				);
			}
			const vk::DescriptorSetLayoutCreateInfo ci(
				{},
				static_cast< uint32_t >( setBindings.size() ),
				setBindings.data()
			);
			return vk::raii::DescriptorSetLayout( cntx.device, ci );
		}

		template <std::ranges::forward_range T>
			requires std::same_as<std::ranges::range_value_t<T>, fgl::vulkan::Buffer>
		[[nodiscard]] vk::raii::DescriptorPool create_descriptor_pool(
			const Context& cntx,
			const T& buffers ) const
		{
			constexpr uint32_t max_sets { 1 };
			constexpr uint32_t pool_size_count { 1 };
			const vk::DescriptorPoolSize poolsize {
				std::ranges::begin( buffers )->buffer_type,
				static_cast< uint32_t >( std::ranges::size( buffers ) )
			};
			const vk::DescriptorPoolCreateInfo ci(
				vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
				max_sets,
				pool_size_count,
				&poolsize
			);
			return vk::raii::DescriptorPool( cntx.device, ci );
		}

		[[nodiscard]]
		vk::raii::PipelineLayout create_pipeline_layout( const Context& cntx ) const;

		[[nodiscard]] vk::raii::Pipeline create_pipeline(
			const Context& cntx,
			const std::string init_function_name
		) const;

		[[nodiscard]]
		vk::raii::DescriptorSets create_descriptor_sets( const Context& cntx ) const;

	public:

		vk::raii::ShaderModule shader_module;
		vk::raii::DescriptorSetLayout descriptor_set_layouts;
		vk::raii::DescriptorPool pool;
		vk::raii::PipelineLayout layout;
		vk::raii::Pipeline pipeline;
		vk::raii::DescriptorSets sets;

		Pipeline() = delete;

		template <std::ranges::forward_range T>
			requires std::same_as<std::ranges::range_value_t<T>, fgl::vulkan::Buffer>
		[[nodiscard]] explicit
			Pipeline(
				const Context& cntx,
				const std::filesystem::path& shaderpath,
				const std::string& shader_init_name,
				const T& buffers )
			:
			shader_module( create_shader_module( cntx, shaderpath ) ),
			descriptor_set_layouts( create_descriptor_set_layout( cntx, buffers ) ),
			pool( create_descriptor_pool( cntx, buffers ) ),
			layout( create_pipeline_layout( cntx ) ),
			pipeline( create_pipeline( cntx, shader_init_name ) ),
			sets( create_descriptor_sets( cntx ) )
		{
			std::vector<vk::WriteDescriptorSet> writeset;
			std::vector<vk::DescriptorBufferInfo> bufferinfo;

			const auto number_of_buffers { std::ranges::size( buffers ) };
			writeset.resize( number_of_buffers );
			bufferinfo.resize( number_of_buffers );

			/// magic values
			constexpr uint32_t offset { 0 };
			constexpr vk::DescriptorImageInfo* imgpointer { nullptr };
			constexpr uint32_t array_element { 0 };
			constexpr uint32_t descriptor_count { 1 };

			/* Simultaneously iterate over buffers, bufferinfo, and writeset.
				Constructs and assigns elements which are associated by order.
				Both writeset and info are constructed from a buffer.
				writeset requires the info's address.*/
			for( const auto& [buffer, buffer_info, write_set]
				: fgl::zip( buffers, bufferinfo, writeset ) )
			{
				buffer_info = vk::DescriptorBufferInfo(
					*buffer.buffer, offset, buffer.bytesize
				);

				write_set = vk::WriteDescriptorSet(
					*sets.front(),
					buffer.binding,
					array_element,
					descriptor_count,
					buffer.buffer_type,
					imgpointer,
					&buffer_info
				);
			}

			cntx.device.updateDescriptorSets( writeset, nullptr );
			//std::cout << "\n\tConstructed Pipeline with " << buffers.size() << " buffers." << std::endl;
		}
	};



}




#endif /* FGL_VULKAN_PIPELINE_HPP_INCLUDE */
