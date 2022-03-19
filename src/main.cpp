#include <algorithm> // find_if
#include <array>
#include <bitset>
#include <cstdint>
#include <cstdlib> // abort, EXIT_SUCCESS
#include <filesystem>
#include <fstream>
#include <sstream> // used for API version exception
#include <utility> // move, pair

#include <iostream> // cout, cerr, endl
#include <string_view>

#include <vulkan/vulkan_raii.hpp>

#include "stopwatch.hpp"

#include <fgl/vulkan.hpp>

// could use StructureChain, but it would be more verbose?
// https://github.com/KhronosGroup/Vulkan-Hpp/search?q=StructureChain

auto create_command_pool( const vk::raii::Device& device, const uint32_t queue_family_index )
{
	const vk::CommandPoolCreateInfo ci( {}, queue_family_index );
	return vk::raii::CommandPool( device, ci );
}

auto createCommandBuffers(
	const vk::raii::Device& device,
	const vk::raii::CommandPool& command_pool,
	const uint32_t buffer_count = 1 )
{
	const vk::CommandBufferAllocateInfo alloc_info(
		*command_pool, vk::CommandBufferLevel::ePrimary,
		buffer_count );
	return vk::raii::CommandBuffers( device, alloc_info );
}

auto create_command_buffer(
	const vk::raii::Device& device,
	const vk::raii::CommandPool& command_pool )
{
	return std::move(
		createCommandBuffers( device, command_pool ).front() );
}

auto create_waitable_fence(
	const vk::raii::Device& device,
	const vk::raii::CommandBuffer& command_buffer,
	const uint32_t queue_family_index,
	const uint32_t queue_index = 0 )
{
	vk::raii::Fence fence { device.createFence( {} ) };
	const vk::raii::Queue queue { device.getQueue(
		queue_family_index, queue_index ) };
	const vk::SubmitInfo submit_info(
		nullptr, nullptr, *command_buffer, nullptr );
	queue.submit( submit_info, *fence );
	return fence;
}

int main()
try
{

	stopwatch::Stopwatch mainwatch( "Main" );
	mainwatch.start();

	fgl::vulkan::AppInfo info(
		VK_API_VERSION_1_1, { "VK_LAYER_KHRONOS_validation" }, {}, 1, 0.0 );

	fgl::vulkan::Context inst( info, true );

	constexpr size_t elements = 512;
	constexpr vk::DeviceSize insize =
		elements * sizeof( uint32_t ) + sizeof( uint32_t );
	constexpr vk::DeviceSize outsize =
		( elements * elements ) * sizeof( uint32_t );
	constexpr size_t invocationsPerDispatch = 2;
	constexpr size_t dispatchNum = elements / invocationsPerDispatch;
	constexpr size_t totalsize = insize + outsize;

	// TODO: Add some security checks to ensure we are not
	// dispatching too many calls and allocating too much
	// memory

	// assert( invocationsPerDispatch *
	// invocationsPerDispatch <
	// inst.properties.limits.maxComputeWorkGroupInvocations
	// ); //Too many invocationsPerDispatch

	if ( invocationsPerDispatch * invocationsPerDispatch >=
		 inst.properties.limits.maxComputeWorkGroupInvocations )
	{
		throw std::runtime_error(
			"Too many invocations per dispatch. Lower the elements count" );
	}

	// assert( totalsize <
	// inst.properties.limits.maxMemoryAllocationCount );
	// //Too many elements allocated

	if ( dispatchNum >=
		 inst.properties.limits.maxComputeWorkGroupCount.front() )
	{
		throw std::runtime_error(
			"dispatch number is too high for supported GPU. Lower the elements count" );
	}

	// assert( dispatchNum <
	// inst.properties.limits.maxComputeWorkGroupCount.front()
	// );

	// Allocate a single memory segment for the buffers
	// being passed in
	/*

	AMD and Nvidia have seperate heap types however...
	AMD DOES have the 256MB cluster of memory that is
	extremely fast for both CPU and GPU (TODO:Research
	resizable bar for AMD gpus.) Perhaps ditching the idea
	of allocating buffers into a single heap would be a
	better idea.
	*/


	// eHostVisible is 'slower' then non eHostVisible memory
	// on certian GPUs. If the memory is not needed to be
	// access on the cpu IE: inter-communication/transfer
	// between two shaders then it doesn't need to be visible


	/*
	Flag meanings for vk::MemoryPropertyFlags

	eDeviceLocal : Memory that is optimal for the GPU only,
	Not mappable. eHostVisible : Memory that can be mapped
	to the host via vkMapMemory eHostCoherent : Memory that
	specieis host cache management (If enabled vkFlush and
	vkInvalidate at nore required) eHostCached : Memory that
	is cached on the host.

	There is a bunch more but so far they are not important
	for what I do.
	*/


	constexpr vk::MemoryPropertyFlags flags =
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent;

	std::vector<fgl::vulkan::Buffer> buffers;
	buffers.reserve( 2 );

	// binding : set

	buffers.emplace_back(
		inst, insize, vk::BufferUsageFlagBits::eStorageBuffer,
		vk::SharingMode::eExclusive, 0, flags,
		vk::DescriptorType::eStorageBuffer );
	buffers.emplace_back(
		inst, outsize, vk::BufferUsageFlagBits::eStorageBuffer,
		vk::SharingMode::eExclusive, 1, flags,
		vk::DescriptorType::eStorageBuffer );


	fgl::vulkan::Pipeline vpipeline(
		inst, std::filesystem::path( "Square.spv" ),
		std::string( "main" ), buffers );

	{
		void* test = buffers.at( 0 ).get_memory();

		// TODO range wrapper
		uint32_t* const in_buffer_data {
			reinterpret_cast<uint32_t*>( test ) };

		uint32_t& matrixsize = in_buffer_data[ 0 ];
		uint32_t* matrix	 = &in_buffer_data[ 1 ];

		matrixsize = elements;

		for ( size_t i = 0; i < elements; ++i )
		{
			matrix[ i ] = static_cast<uint32_t>( i );
		}

		/*std::cout << "Input Buffer:" << std::endl;
		for( size_t i = 0; i < elements + 1; ++i )
		{
			std::cout << std::setw( 5 ) << in_buffer_data[i]
		<< " ";
		}
		std::cout << std::endl;*/


		buffers.at( 0 ).memory.unmapMemory();
	}

	const auto command_pool { create_command_pool(
		inst.device, inst.queue_family_index ) };

	const auto command_buffer { create_command_buffer(
		inst.device, command_pool ) };

	const vk::CommandBufferBeginInfo bi {
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
	command_buffer.begin( bi );
	command_buffer.bindPipeline(
		vk::PipelineBindPoint::eCompute, *vpipeline.pipeline );

	std::vector<vk::DescriptorSet> array;


	for ( const auto& tlayout : vpipeline.sets )
	{
		array.push_back( *tlayout );
	}

	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute, *vpipeline.layout,
		0, array, nullptr );
	command_buffer.dispatch( dispatchNum, dispatchNum, 1 );
	command_buffer.end();


	const auto fence { create_waitable_fence(
		inst.device, command_buffer, inst.queue_family_index ) };
	constexpr uint64_t timeout { 5 };
	while ( vk::Result::eTimeout ==
			inst.device.waitForFences( { *fence }, VK_TRUE, timeout ) )
		;

	// constexpr vk::DeviceSize out_map_offset { 0 };
	auto out_buffer_ptr = reinterpret_cast<uint32_t*>(
		buffers.at( 1 ).get_memory() );

	/// PRINT
	/*std::cout << "Output Buffer:" << std::endl;
	for( size_t y = 0; y < elements; ++y )// spammy...
	{
		for( size_t x = 0; x < elements; ++x )
		{
			auto index = y * elements + x;
			std::cout << std::setw( 5 ) <<
	out_buffer_ptr[index];
		}
		std::cout << "\n\n" << std::endl;
	}*/

	//
	/// PRINT
	buffers.at( 1 ).memory.unmapMemory();

	mainwatch.stop();
	std::cout << '\n' << mainwatch << std::endl;
}
catch ( const vk::SystemError& e )
{
	std::cerr << "\n\n Vulkan system error code:\t" << e.code()
			  << "\n\t error:" << e.what() << std::endl;
	std::abort();
}
catch ( const std::exception& e )
{
	std::cerr << "\n\n Exception caught:\n\t" << e.what()
			  << std::endl;
	std::abort();
}
catch ( ... )
{
	std::cerr << "\n\n An unknown error has occured.\n";
	std::abort();
}
