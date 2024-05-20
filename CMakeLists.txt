cmake_minimum_required(VERSION 3.9)
project(ICon7)
cmake_policy(SET CMP0115 OLD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
else()
endif()

option(ICON7_BUILD_EXAMPLES "Build icon7 examples" ON)
option(ICON7_BUILD_TEST "Build icon7 tests" ON)
option(ICON7_USE_RPMALLOC "Use rpmalloc. Otherwise use object pool" ON)
option(ICON7_MEMORY_COLLECT_STATS "Enable memory statistics collection" OFF)
option(ICON7_USE_MALLOC_TRIM "Enable malloc trimming with each call to free()" OFF)
option(ICON7_LOG_LEVEL_SINGLE_CHARACTER "Write single character for ICon7 log level" ON)
option(ICON7_LOG_USE_FILENAME_WITHOUT_PATH "Write file names without full path to ICon7 log" ON)
option(ICON7_LOG_USE_PRETTY_FUNCTION "Use pretty function name for ICon7 log" ON)
set(ICON7_LOG_DATETIME_SUBSECONDS_DIGITS 5 CACHE STRING "Number of sub-second digits in ICon7 log date-time")

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

if (ICON7_USE_RPMALLOC)
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

if (NOT DEFINED ICON7_IGNORE_COMMON_LOG_PATH)
target_compile_definitions(icon7 PUBLIC ICON7_LOG_IGNORE_COMMON_PATH="${PROJECT_SOURCE_DIR}")
endif()
if(NOT DEFINED ICON7_LOG_DEFAULT_LOG_LEVEL)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=INFO)
	else()
		target_compile_definitions(icon7 PUBLIC ICON7_LOG_DEFAULT_LOG_LEVEL=IGNORE)
	endif()
endif()

if(ICON7_MEMORY_COLLECT_STATS)
	target_compile_definitions(icon7 PUBLIC ICON7_MEMORY_COLLECT_STATS=1)
endif()

if(ICON7_USE_MALLOC_TRIM)
	target_compile_definitions(icon7 PUBLIC ICON7_USE_MALLOC_TRIM=1)
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
	add_executable(stress_test
		tests/stress_test
	)
	target_link_libraries(stress_test icon7)
	
	add_executable(multi_test
		tests/multi_testing
	)
	target_link_libraries(multi_test icon7)
endif()

if(ICON7_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
