cmake_minimum_required(VERSION 3.9)
project(ICon7)
cmake_policy(SET CMP0115 OLD)

include (precompile_stl_and_lib_headers.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_COLOR_DIAGNOSTICS ON)

option(ENABLE_LTO "Enable LTO" OFF)
option(ICON7_BUILD_EXAMPLES "Build icon7 examples" ON)
option(ICON7_BUILD_TEST "Build icon7 tests" ON)
option(ICON7_CLANG_FUZZTEST "Compile fuzztesting and use clang" OFF)
option(USOCKETS_USE_OPENSSL "Use OpenSSL instead of BoringSSL" ON)

if(ICON7_CLANG_FUZZTEST)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-instr-generate -fcoverage-mapping")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -fprofile-instr-generate -fcoverage-mapping")
endif()

if(ENABLE_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_err LANGUAGES CXX)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

add_subdirectory(drwalin_util)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

add_subdirectory(bitscpp)

target_precompile_headers(bitscpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/icon7/ByteBuffer.hpp")

include_directories(include/)
include_directories(uSockets/src/)
include_directories(bitscpp/include/)
include_directories(${ZLIB_INCLUDE_DIRS})
if(USOCKETS_USE_OPENSSL)
	include_directories(${OPENSSL_INCLUDE_DIR})
	
	find_package(OpenSSL REQUIRED)
else()
	add_subdirectory(uSockets/boringssl EXCLUDE_FROM_ALL)
	include_directories(uSockets/boringssl/include)
endif()

find_package(ZLIB REQUIRED)

aux_source_directory(uSockets/src/ usockets_files)
aux_source_directory(uSockets/src/eventing usockets_files_eventing)
add_library(uSockets STATIC
	uSockets/src/crypto/openssl.c
	uSockets/src/crypto/sni_tree.cpp
	${usockets_files}
	${usockets_files_eventing}
)
target_link_libraries(uSockets pthread ${ZLIB_LIBRARIES} m)

if(UNIX)
# 	add_subdirectory(libuv)
# 	target_link_libraries(uSockets uv_a)
# 	target_compile_definitions(uSockets PRIVATE LIBUS_USE_LIBUV)
else()
	add_subdirectory(libuv)
	target_link_libraries(uSockets uv_a ws2_32)
	target_compile_definitions(uSockets PRIVATE LIBUS_USE_LIBUV)
endif()

if(USOCKETS_USE_OPENSSL)
	target_link_libraries(uSockets ${OPENSSL_LIBRARIES} crypto)
	target_compile_definitions(uSockets PRIVATE WITH_OPENSSL)
else()
	target_compile_definitions(uSockets PRIVATE WITH_BORINGSSL)
	target_link_libraries(uSockets crypto ssl)
endif()

target_compile_definitions(uSockets PRIVATE LIBUS_USE_OPENSSL)

aux_source_directory(./include/icon7/ header_files)
aux_source_directory(./src source_files)

add_library(icon7 STATIC
	${header_files}
	${source_files}
)
target_link_libraries(icon7 PUBLIC uSockets)
target_link_libraries(icon7 PUBLIC bitscpp)
target_link_libraries(icon7 PUBLIC drwalin_util)
add_common_precompiled_headers(icon7)

if(ASAN)
	add_definitions(-fsanitize=address)
endif()

if(ICON7_BUILD_TEST)
	add_executable(multi_test
		tests/multi_testing
	)
	target_link_libraries(multi_test icon7)

	add_executable(test_flat_hash_map
		tests/test_flat_hash_map.cpp
	)
	target_link_libraries(test_flat_hash_map icon7)
endif()

if(ICON7_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(ICON7_CLANG_FUZZTEST)
	add_executable(fuzz_test
		tests/fuzz_test
	)
	target_compile_options(fuzz_test
		PRIVATE -fsanitize=address,fuzzer
	)
	target_link_libraries(fuzz_test
		PRIVATE -fsanitize=address,fuzzer
		icon7
	)

	add_executable(fuzz_test1
		tests/fuzz_test_threaded.cpp
	)
	target_compile_options(fuzz_test1
		PRIVATE -fsanitize=address,fuzzer
	)
	target_link_libraries(fuzz_test1
		PRIVATE -fsanitize=address,fuzzer
		icon7
	)

	add_executable(fuzz_test2
		tests/fuzz_test_coroutined.cpp
	)
	target_compile_options(fuzz_test2
		PRIVATE -fsanitize=address,fuzzer
	)
	target_link_libraries(fuzz_test2
		PRIVATE -fsanitize=address,fuzzer
		icon7
	)

	add_executable(fuzz_test3
		tests/fuzz_test_manual_iterations.cpp
	)
	target_compile_options(fuzz_test3
		PRIVATE -fsanitize=address,fuzzer
	)
	target_link_libraries(fuzz_test3
		PRIVATE -fsanitize=address,fuzzer
		icon7
	)
endif()
