function(add_common_precompiled_headers target)
	target_precompile_headers(${target} PUBLIC <map> <unordered_map> <vector>
		<string> <functional> <string_view> <fstream> <sstream> <thread>
		<atomic> <mutex> <queue> <list> <array> <valarray> <set> <unordered_set>
		<algorithm> <ios> <ostream> <streambuf> <utility> <stack> <new> <memory>
		<limits> <complex> <bitset> <locale> <ostream> <istream> <future>
		<tuple> <regex> <type_traits> <forward_list> <initializer_list>
		<condition_variable> <random> <chrono> <shared_mutex> <filesystem>
		<memory_resource> <cstring> <cstdio> <cmath> <cinttypes> <cfloat>
		<climits> <cstdlib> <execution> <compare> <coroutine>
		<csignal> <cstdarg> <cstddef> <cstdint> <any> <variant> <bit> <bitset>
		<valarray> <expected> <optional> <variant> <array> <queue>
		<flat_map> <flat_set> <initializer_list> <algorithm> <forward_list>
		<list> <span> <variant> <type_traits> <memory_resource>
		<scoped_allocator> <cassert> <cerrno> <stdexcept> <system_error> <new>
		<limits> <version> <typeinfo> <typeindex> <source_location> <typeinfo>
		<numeric> <cctype> <clocale> <ctime> <chrono> <regex> <format> <complex>
		<random> <numbers> <streambuf> <syncstream> <future> <latch>
		<semaphore> <stop_token>)

	target_precompile_headers(${target} PUBLIC
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/concurrentqueue/concurrentqueue.h"
	)

	file(GLOB_RECURSE files "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bitscpp/include/*")
	target_precompile_headers(${target} PUBLIC ${files})
endfunction()
