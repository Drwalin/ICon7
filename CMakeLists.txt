cmake_minimum_required(VERSION 3.8)
project(ICon7)
cmake_policy(SET CMP0115 OLD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ICON7_BUILD_EXAMPLES "Build examples" ON)
option(ICON7_BUILD_TEST "Build tests" ON)

add_subdirectory(GameNetworkingSockets PRIVATE)

add_subdirectory(bitscpp)

include_directories(./include/)
include_directories(./GameNetworkingSockets/include/)
include_directories(./bitscpp/include/)

aux_source_directory(./include/icon7/ header_files)
aux_source_directory(./src source_files)

add_library(icon7 STATIC
	${header_files}
	${source_files}
)
target_link_libraries(icon7 GameNetworkingSockets::shared)

if(ICON7_BUILD_TEST)
	add_executable(stress_test
		tests/stress_test
	)
	target_link_libraries(stress_test icon7)
endif()

if(ICON7_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

