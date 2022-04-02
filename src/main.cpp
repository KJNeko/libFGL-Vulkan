#include <cstdlib> // abort, EXIT_SUCCESS
#include <cstdint>
#include <array>
#include <utility> // move, pair
#include <algorithm> // find_if
#include <sstream> // used for API version exception
#include <filesystem>
#include <fstream>
#include <bitset>
#include <thread>
#include <chrono>

#include <string_view>
#include <iostream> // cout, cerr, endl

#include <vulkan/vulkan_raii.hpp>

#include "stopwatch.hpp"

#include <fgl/vulkan.hpp>

#include <fgl/types/range_alias.hpp>

// could use StructureChain, but it would be more verbose?
// https://github.com/KhronosGroup/Vulkan-Hpp/search?q=StructureChain

// could use StructureChain, but it would be more verbose?
// https://github.com/KhronosGroup/Vulkan-Hpp/search?q=StructureChain

auto create_command_pool(
	const vk::raii::Device& device,
	const uint32_t queue_family_index )
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
		*command_pool, vk::CommandBufferLevel::ePrimary, buffer_count
	);
	return vk::raii::CommandBuffers( device, alloc_info );
}

auto create_command_buffer(
	const vk::raii::Device& device,
	const vk::raii::CommandPool& command_pool )
{
	return std::move( createCommandBuffers( device, command_pool ).front() );
}

auto create_waitable_fence(
	const vk::raii::Device& device,
	const vk::raii::CommandBuffer& command_buffer,
	const uint32_t queue_family_index,
	const uint32_t queue_index = 0 )
{
	vk::raii::Fence fence { device.createFence( {} ) };
	const vk::raii::Queue queue { device.getQueue( queue_family_index, queue_index ) };
	const vk::SubmitInfo submit_info( nullptr, nullptr, *command_buffer, nullptr );
	queue.submit( submit_info, *fence );
	return fence;
}


//ORDER
/*
 * CPU
 * GPUTIME
 * GPUEXEC
 */

std::vector<size_t> counts;
std::vector<std::chrono::nanoseconds> times;



void gpu(unsigned int lookFor, std::vector<unsigned int> data)
{
	auto start = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> gpuExec;

	//Start timer here

	fgl::vulkan::AppInfo info(
		VK_API_VERSION_1_2,
		{ "VK_LAYER_KHRONOS_validation" },
		{},
		1,
		0.0
	);

	fgl::vulkan::Context inst( info );

	constexpr vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

	std::vector<fgl::vulkan::Buffer> buffers;
	buffers.reserve(1);

	buffers.emplace_back(inst, sizeof(unsigned int) * (data.size() + 2), vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 0, flags, vk::DescriptorType::eStorageBuffer);

	fgl::vulkan::Pipeline vpipeline(inst, std::filesystem::path("Square.spv"), std::string("main"), buffers);


	//Memory access scope
	{
		void* bufferPtr = buffers.at(0).get_memory();

		uint32_t* const in_buffer_data{ reinterpret_cast<uint32_t*>(bufferPtr)};

		uint32_t& maxSize = in_buffer_data[0];
		uint32_t& lookForValue = in_buffer_data[1];

		maxSize = static_cast<unsigned int>(data.size());
		lookForValue = lookFor;

		uint32_t* data = &in_buffer_data[2];

		std::cout << "\n\nGPU ARRAY INFO:\n\tARRAY SIZE: " << maxSize << "\n\tLOOKING FOR: " << lookForValue << std::endl;
	}

	const auto command_pool {create_command_pool(inst.device, inst.queue_family_index)};

	const auto command_buffer{create_command_buffer(inst.device, command_pool)};

	const vk::CommandBufferBeginInfo bi{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

	command_buffer.begin(bi);
	command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, *vpipeline.pipeline);

	std::vector<vk::DescriptorSet> setArray;

	for(const auto& tlayout : vpipeline.sets)
	{
		setArray.push_back(*tlayout);
	}

	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *vpipeline.layout, 0, setArray, nullptr);
// Start the timer here

	gpuExec = std::chrono::steady_clock::now();

	command_buffer.dispatch(1,1,1);
	command_buffer.end();

	const auto fence {create_waitable_fence(inst.device, command_buffer, inst.queue_family_index)};

	constexpr uint64_t timeout{5};

	while(vk::Result::eTimeout == inst.device.waitForFences({*fence}, VK_TRUE, timeout));

//ENDTIME
	auto end = std::chrono::steady_clock::now();

	auto totalTime = end - start;
	auto executeTime = end - gpuExec;

	times.push_back(totalTime);
	times.push_back(executeTime);
}

void cpu(unsigned int lookFor, std::vector<unsigned int> data)
{
	std::cout << "\n\nCPU ARRAY INFO:\n\tARRAY SIZE: " << data.size() << "\n\tLOOKING FOR: " << lookFor << std::endl;
	//Start the find

	auto start = std::chrono::steady_clock::now();

	bool found = false;
	for(const auto& thing : data)
	{
		if(lookFor == thing)
		{
			found = true;
			break;
		}
	}

	auto point = std::chrono::steady_clock::now();

	times.push_back(point - start);

	if(found)
	{
		std::cout << "Found the value" << std::endl;
	}

}

std::pair<unsigned int, std::vector<unsigned int>> getData(size_t size, size_t divisor)
{
	std::vector<unsigned int> data;
	data.resize(size);

	for(size_t i = 0; i < size; ++i)
	{
		data.at(i) = i;
	}

	//unsigned int lookFor = data.at(size / divisor);
	unsigned int lookFor = data.back();


	return std::pair(lookFor, data);
}

uint32_t printDebug()
{
	fgl::vulkan::AppInfo info(
		VK_API_VERSION_1_2,
		{ "" },
		{},
		1,
		0.0
	);

	fgl::vulkan::Context inst( info );

	inst.print_debug_info();

	return inst.physical_device.getProperties().limits.maxStorageBufferRange;
}


int main() try
{
	using namespace std::literals::chrono_literals;

	auto maxBuffer = printDebug();


	for(size_t i = 0; i < 10; ++i)
	{
		for(size_t mult = 1; mult < 6000000000 / sizeof(unsigned int); mult *= 2)
		{
			if(mult * sizeof(unsigned int) > maxBuffer)
			{
				std::cout << "Test is over the maxStorageBufferRange of the gpu. Ignoring" << std::endl;
				continue;
			}

			counts.push_back(mult);

			auto data = getData(mult,2);

			cpu(data.first, data.second);
			std::this_thread::yield();
			gpu(data.first, data.second);
			std::this_thread::yield();

			std::cout << std::setfill('=') << std::setw(80) << ' ' << std::setfill(' ') << std::endl;

		}
	}


	std::cout << "Printing times: "<< std::endl;

	for(size_t i = 0; i < counts.size(); ++i)
	{
		size_t offset = i * 3;
		std::cout << "Count: " << counts.at(i) <<"\t\t"<< std::setw(40)<< "CPUTime: " << times.at(offset).count()<< std::setw(40)<< "GPUTotal: " << times.at(offset + 1).count() <<std::setw(40)<< "GPUExec: " << times.at(offset + 2).count() << std::endl;
	}

	std::cout << "RAW" << std::endl;

	for(size_t i = 0; i < counts.size(); ++i)
	{
		size_t offset = i * 3;
		std::cout << counts.at(i) << "\t" << times.at(offset).count() << "\t" << times.at(offset + 1).count() <<"\t" << times.at(offset + 2).count() << std::endl;
	}

	/*
	 0 | 0,1,2
	 1 | 3,4,5
	 2 | 6,7,8
	 *
	 *
	 */


	return 0;
}
catch( const vk::SystemError& e )
{
	std::cerr << "\n\n Vulkan system error code:\t" << e.code() << "\n\t error:" << e.what() << std::endl;
	std::abort();
}
catch( const std::exception& e )
{
	std::cerr << "\n\n Exception caught:\n\t" << e.what() << std::endl;
	std::abort();
}
catch( ... )
{
	std::cerr << "\n\n An unknown error has occured.\n";
	std::abort();
}
