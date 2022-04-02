#include <vulkan/vulkan_raii.hpp>

#include <fgl/vulkan/memory.hpp>
#include <fgl/vulkan/context.hpp>

namespace fgl::vulkan
{
	size_t Buffer::bytecount { 0 }; //Static member of Buffer

	namespace internal
	{
		//BUFFER
		vk::raii::Buffer create_buffer(
			const Context& vulkan,
			const vk::DeviceSize size,
			const vk::BufferUsageFlagBits usageflags,
			const vk::SharingMode sharingmode )
		{
			constexpr uint32_t number_of_family_indexes { 1 };

			const auto index { vulkan.index_of_first_queue_family( vk::QueueFlagBits::eCompute ) };

			const vk::BufferCreateInfo ci(
				{},
				size,
				usageflags,
				sharingmode,
				number_of_family_indexes,
				&index
			);
			return vulkan.device.createBuffer( ci );
		}

		std::pair<uint32_t, vk::DeviceSize> get_memory_type(
			const vk::PhysicalDeviceMemoryProperties& device_memory_properties,
			const vk::MemoryPropertyFlags flags )
		{
			for(
				uint32_t current_memory_index = 0;
				current_memory_index < device_memory_properties.memoryTypeCount;
				++current_memory_index )
			{
				const vk::MemoryType memoryType {
					device_memory_properties.memoryTypes[current_memory_index]
				};
				if( flags & memoryType.propertyFlags )
				{
					const vk::DeviceSize memory_heap_size {
						 device_memory_properties.memoryHeaps[memoryType.heapIndex].size
					};
					return std::pair { current_memory_index, memory_heap_size };
				}
			}
			throw std::runtime_error( "Failed to get memory type." );
		}

		vk::raii::DeviceMemory create_device_memory(
			const Context& context,
			const vk::raii::Buffer& buffer,
			const vk::DeviceSize bytesize,
			const vk::MemoryPropertyFlags flags )
		{
			const auto& [memindex, size] { get_memory_type( context.physical_device.getMemoryProperties(), flags ) };

			auto& bytecount { Buffer::bytecount };

			bytecount += buffer.getMemoryRequirements().size;
			if( context.physical_device.getMemoryProperties().memoryHeaps[1].size < bytecount )
			{
				std::stringstream ss;
				ss
					<< "Attempting to allocate too much memory (in Byte)\n"
					<< "\tMemory allocated: " << bytesize << "\n"
					<< "\tMaximum Memory: " << context.physical_device.getMemoryProperties().memoryHeaps[1].size << "\n"
					<< "\tMemory avalilable: " << context.properties.limits.maxMemoryAllocationCount - ( bytecount - bytesize ) << "\n"
					<< "\tMemory Over: " << bytecount - context.physical_device.getMemoryProperties().memoryHeaps[1].size - ( bytecount - bytesize ) << "\n";

				throw std::runtime_error( ss.str() );
			}

			const vk::MemoryAllocateInfo memInfo( buffer.getMemoryRequirements().size, memindex );

			return context.device.allocateMemory( memInfo );
		}

	} // namespace internal


	Buffer::Buffer(
		const Context& context,
		const vk::DeviceSize& size,
		const vk::BufferUsageFlagBits usageflags,
		const vk::SharingMode sharingmode,
		const uint32_t binding_,
		const vk::MemoryPropertyFlags flags,
		const vk::DescriptorType type )
		:
		binding( binding_ ),
		buffer_type( type ),
		bytesize( size ),
		buffer( internal::create_buffer( context, size, usageflags, sharingmode ) ),
		memory( internal::create_device_memory( context, buffer, bytesize, flags ) )
	{
		constexpr vk::DeviceSize offset { 0 };
		buffer.bindMemory( *memory, offset );
		/*std::cout
			<< "\n\tAllocated " << size << " bytes to binding: " << binding_
			<< std::endl;*/
	}

	void* Buffer::get_memory() const
	{
		constexpr vk::DeviceSize offset { 0 };
		return memory.mapMemory( offset, bytesize );
	}

}
