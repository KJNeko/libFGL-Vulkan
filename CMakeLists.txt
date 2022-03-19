cmake_minimum_required(VERSION 3.14)



if(WIN32)
    project(VulkanCompute LANGUAGES CXX)
elseif(UNIX)
    project(VulkanCompute)
endif()

set(CMAKE_CONFIGURATION_TYEPS "Release;Dev" CACHE STRING "" FORCE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
# -fsanitize=address 
# -fmodules-ts

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Og -DDEBUG -lpqxx -lssl -lcrypto -lpq -Werror=cpp -Wall -Wextra -pthread -pedantic -fmax-errors=3 -Wundef -Wnull-dereference -Wduplicated-cond -Wduplicated-branches -Wshadow -Wswitch -Wswitch-enum -Wstrict-overflow=5 -Wconversion -Wfloat-equal -Wdouble-promotion -Wwrite-strings -Wpointer-arith -Wcast-qual -Wformat=2 -Wformat-signedness -Wformat-y2k -Wformat-overflow=2 -Wold-style-cast -Wnon-virtual-dtor -Wsuggest-final-types -Wsuggest-final-methods -Wsuggest-override -Wsized-deallocation -Wextra-semi -Wmultiple-inheritance -Wzero-as-null-pointer-constant -Werror") #-fuse-linker-plugin -flto -fno-fat-lto-objects")
#-fanalyzer 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I'./include/' -I'./libFGL/include' -Ofast -std=c++20 -lpthread -lvulkan -ldl -fmax-errors=3 -Wall -Wextra -Wundef -Wnull-dereference -Wpedantic -pedantic-errors -Wuninitialized -Wunused -Wunused-parameter -Winit-self -Wconversion -Wuseless-cast -Wextra-semi -Wsuggest-final-types -Wsuggest-final-methods -Wsuggest-override  -Wformat-signedness -Wno-format-zero-length -Wmissing-include-dirs -Wshift-overflow=2 -Walloc-zero -Walloca -Wsign-promo -Wconversion -Wduplicated-branches -Wduplicated-cond -Wfloat-equal -Wshadow -Wshadow=local -Wmultiple-inheritance -Wvirtual-inheritance -Wno-virtual-move-assign -Wunsafe-loop-optimizations -Wenum-conversion -Wnormalized -Wpacked -Wredundant-decls -Wmismatched-tags -Wredundant-tags -Wctor-dtor-privacy -Wdeprecated-copy-dtor -Wstrict-null-sentinel -Wold-style-cast -Woverloaded-virtual -Wzero-as-null-pointer-constant -Wconditionally-supported -Werror=pedantic -Wwrite-strings -Wmultiple-inheritance -Wunused-const-variable=2 -Wdouble-promotion -Warith-conversion -Wpointer-arith -Wcast-qual -Wconversion -Wsign-conversion -Wimplicit-fallthrough=1 -Wmisleading-indentation -Wdangling-else -Wdate-time -Wformat=2 -Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2 -Wswitch-default -Wswitch-enum -Wstrict-overflow=5 -Wstringop-overflow=4 -Warray-bounds=2 -Wattribute-alias=2 -Wcatch-value=2 -Wplacement-new=2 -Wtrampolines")



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MAKE_PROGRAM make)


find_package(Vulkan REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#inclusion of project files#
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")

include_directories(${INCLUDE_DIR})
include_directories(${SOURCE_DIR})

add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/Sort.spv"
    #COMMAND /home/kj16609/Desktop/1.2.198.1/x86_64/bin/dxc -T cs_6_0 -E "Main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.1 -Fo "${SOURCE_DIR}/Square.spv" "${SOURCE_DIR}/Square.hlsl"
    COMMAND glslc "${SOURCE_DIR}/Sort.comp" -o "${CMAKE_SOURCE_DIR}/Sort.spv"
    DEPENDS "${SOURCE_DIR}/Sort.comp"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding Shaders"
)

add_custom_target(ComputeShader DEPENDS "${CMAKE_SOURCE_DIR}/Sort.spv")

file(GLOB_RECURSE SOURCES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.hlsl"
    "${SOURCE_DIR}/*.comp"
)

##SETUP EXE##

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS})

if(WIN32)
    add_executable(VulkanCompute WIN32 ${SOURCES})
elseif(UNIX)
    add_executable(VulkanCompute ${SOURCES})
endif()

target_include_directories(VulkanCompute PRIVATE ${INCLUDE_DIR})
target_include_directories(VulkanCompute PRIVATE ${SOURCE_DIR})

add_dependencies(VulkanCompute ComputeShader)
