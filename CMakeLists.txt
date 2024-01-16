cmake_minimum_required(VERSION 3.8)
project(ICon7)
cmake_policy(SET CMP0115 OLD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ICON7_BUILD_EXAMPLES "Build examples" ON)
option(ICON7_BUILD_TEST "Build tests" ON)

add_subdirectory(bitscpp)

include_directories(include/)
include_directories(uSockets/src/)
include_directories(bitscpp/include/)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

aux_source_directory(uSockets/src/ usockets_files)
aux_source_directory(uSockets/src/eventing usockets_files_eventing)
aux_source_directory(uSockets/src/crypto usockets_files_crypto)
add_library(uSockets STATIC
	${usockets_files}
	${usockets_files_eventing}
	${usockets_files_crypto}
)
target_link_libraries(uSockets ${OPENSSL_LIBRARIES} crypto pthread ${ZLIB_LIBRARIES})

target_compile_definitions(uSockets PRIVATE LIBUS_USE_OPENSSL)


aux_source_directory(./include/icon7/ header_files)
aux_source_directory(./src source_files)

add_library(icon7 STATIC
	${header_files}
	${source_files}
)
target_link_libraries(icon7 uSockets)

if(ICON7_BUILD_TEST)
	add_executable(stress_test
		tests/stress_test
	)
	target_link_libraries(stress_test icon7)
endif()

if(ICON7_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

