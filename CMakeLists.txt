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
option(ICON7_USE_RPMALLOC "Use rpmalloc. Otherwise use object pool" ON)
option(ICON7_MEMORY_COLLECT_STATS "Enable memory statistics collection" OFF)
option(ICON7_LOG_LEVEL_SINGLE_CHARACTER "Write single character for ICon7 log level" ON)
option(ICON7_LOG_USE_FILENAME_WITHOUT_PATH "Write file names without full path to ICon7 log" ON)
option(ICON7_LOG_USE_PRETTY_FUNCTION "Use pretty function name for ICon7 log" ON)
option(ICON7_CLANG_FUZZTEST "Compile fuzztesting and use clang" OFF)
option(USOCKETS_USE_OPENSSL "Use OpenSSL instead of BoringSSL" ON)
set(ICON7_LOG_DATETIME_SUBSECONDS_DIGITS 5 CACHE STRING "Number of sub-second digits in ICon7 log date-time")

if(ICON7_CLANG_FUZZTEST)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -g -fprofile-instr-generate -fcoverage-mapping")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O1 -g -fprofile-instr-generate -fcoverage-mapping")
endif()

if(ENABLE_LTO)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

add_subdirectory(concurrent)
add_subdirectory(bitscpp)

include_directories(include/)
include_directories(uSockets/src/)
include_directories(bitscpp/include/)
include_directories(concurrent/)
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
else()
	add_subdirectory(libuv)
	target_link_libraries(uSockets uv_a ws2_32)
	# target_compile_definitions(uSockets PRIVATE LIBUS_USE_LIBUV)
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
target_link_libraries(icon7 uSockets)
target_link_libraries(icon7 concurrent)
add_common_precompiled_headers(icon7)

if(ASAN)
	add_definitions(-fsanitize=address)
endif()

if(ICON7_USE_RPMALLOC)
	target_link_libraries(icon7 rpmalloc)
	add_library(rpmalloc STATIC
		rpmalloc/rpmalloc/rpmalloc.c
	)
	target_compile_definitions(rpmalloc PUBLIC ENABLE_OVERRIDE=0)
	include_directories(rpmalloc/rpmalloc/)
	
	target_compile_definitions(icon7 PUBLIC ICON7_USE_RPMALLOC=1)
else()
	target_compile_definitions(icon7 PUBLIC ICON7_USE_THREAD_CACHED_POOL=0)
endif()

if(NOT DEFINED ICON7_IGNORE_COMMON_LOG_PATH)
target_compile_definitions(icon7 PUBLIC ICON7_LOG_IGNORE_COMMON_PATH="${PROJECT_SOURCE_DIR}")
endif()
if(NOT DEFINED ICON7_LOG_DEFAULT_LOG_LEVEL)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=INFO)
	elseif(ICON7_CLANG_FUZZTEST)
		target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=LL_ERROR)
	else()
		target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=LL_IGNORE)
	endif()
else()
	target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=${ICON7_LOG_DEFAULT_LOG_LEVEL})
endif()

if(ICON7_MEMORY_COLLECT_STATS)
	target_compile_definitions(icon7 PUBLIC ICON7_MEMORY_COLLECT_STATS=1)
endif()

if(ICON7_LOG_LEVEL_SINGLE_CHARACTER)
	target_compile_definitions(icon7 PUBLIC ICON7_LOG_LEVEL_SINGLE_CHARACTER=1)
endif()

if(ICON7_LOG_USE_FILENAME_WITHOUT_PATH)
	target_compile_definitions(icon7 PUBLIC
		ICON7_LOG_USE_FILENAME_WITHOUT_PATH=1)
endif()

if(ICON7_LOG_USE_PRETTY_FUNCTION)
	target_compile_definitions(icon7 PUBLIC ICON7_LOG_USE_PRETTY_FUNCTION=1)
endif()

target_compile_definitions(icon7 PUBLIC
	ICON7_LOG_DATETIME_SUBSECONDS_DIGITS=${ICON7_LOG_DATETIME_SUBSECONDS_DIGITS})

if(ICON7_BUILD_TEST)
	add_executable(multi_test
		tests/multi_testing
	)
	target_link_libraries(multi_test icon7)
	target_compile_definitions(multi_test PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=LL_IGNORE)
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
	
	add_executable(fuzz_test2
		tests/fuzz_test2
	)
	target_compile_options(fuzz_test2
		PRIVATE -fsanitize=address,fuzzer
	)
	target_link_libraries(fuzz_test2
		PRIVATE -fsanitize=address,fuzzer
		icon7
	)
endif()
