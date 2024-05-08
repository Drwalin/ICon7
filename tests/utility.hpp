#pragma once

#include <cstring>

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

template<typename T>
struct _DataStats {
	T mean;
	T stddev;
	T p0;
	T p1;
	T p5;
	T p10;
	T p25;
	T p50;
	T p80;
	T p90;
	T p95;
	T p99;
	T p999;
	T p100;
};
using DataStats = _DataStats<double>;

template<typename T>
_DataStats<T> CalcDataStats(T *ar, size_t count)
{
	_DataStats<T> stats;
	memset(&stats, 0, sizeof(stats));
	if (count < 3) {
		return stats;
	}
	std::sort(ar, ar + count);
	stats.mean = 0;
	for (size_t i = 0; i < count; ++i) {
		stats.mean += ar[i];
	}
	stats.mean /= (T)count;

	stats.stddev = 0;
	for (size_t i = 0; i < count; ++i) {
		T d = ar[i] - stats.mean;
		stats.stddev += d * d;
	}
	stats.stddev = sqrt(stats.stddev / (T)count);

	size_t last = count - 1;
	stats.p0 = ar[(last * 0) / 100];
	stats.p1 = ar[(last * 1) / 100];
	stats.p5 = ar[(last * 5) / 100];
	stats.p10 = ar[(last * 10) / 100];
	stats.p25 = ar[(last * 25) / 100];
	stats.p50 = ar[(last * 50) / 100];
	stats.p80 = ar[(last * 80) / 100];
	stats.p90 = ar[(last * 90) / 100];
	stats.p95 = ar[(last * 95) / 100];
	stats.p99 = ar[(last * 99) / 100];
	stats.p999 = ar[(last * 999) / 1000];
	stats.p100 = ar[last];

	return stats;
}

class ArgsParser
{
public:
	inline ArgsParser(int argc, char **argv)
	{
		progName = argv[0];
		for (int i=1; i<argc; ++i) {
			if (argv[i][0] == '-') {
				std::string s = argv[i];
				auto pos = s.find('=');
				if (pos == std::string::npos) {
					args.emplace_back(s, "");
				} else {
					std::string key = s.substr(0, pos);
					std::string value = s.substr(pos+1);
					args.emplace_back(key, value);
				}
			} else {
				floatingArgs.push_back(argv[i]);
			}
		}
	}
	
	inline bool IsPresent(const std::vector<std::string> &params,
						  bool *hasValue = nullptr) const
	{
		for (int i=0; i<args.size(); ++i) {
			for (int j=0; j<params.size(); ++j) {
				if (args[i].key == params[j]) {
					if (hasValue) {
						*hasValue = args[i].value != "";
					}
					return true;
				}
			}
		}
		if (hasValue) {
			*hasValue = false;
		}
		return false;
	}
	
	inline bool HasValue(const std::vector<std::string> &params)
	{
		bool ret = false;
		IsPresent(params, &ret);
		return ret;
	}
	
	inline std::string GetString(const std::vector<std::string> &params,
								 std::string defaultValue,
								 bool *exists = nullptr) const
	{
		for (int i=0; i<args.size(); ++i) {
			for (int j=0; j<params.size(); ++j) {
				if (args[i].key == params[j]) {
					if (exists) {
						*exists = true;
					}
					return args[i].value;
				}
			}
		}
		if (exists) {
			*exists = false;
		}
		return defaultValue;
	}
	
	inline int64_t GetInt(const std::vector<std::string> &params) const
	{
		bool exists = false;
		auto v = GetString(params, "0", &exists);
		if (exists) {
			return std::stoll(v);
		}
		return 0;
	}
	
	inline int64_t GetInt(const std::vector<std::string> &params,
						  int64_t defaultValue, int64_t min, int64_t max) const
	{
		bool exists = false;
		auto v = GetString(params, "", &exists);
		if (exists) {
			if (v == "")
				return defaultValue;
			int64_t r = std::stoll(v);
			return std::min(std::max(r, min), max);
		}
		return defaultValue;
	}
	
	inline bool GetFlag(const std::vector<std::string> &params) const
	{
		bool ret = false;
		if (IsPresent(params, &ret)) {
			if (ret) {
				int64_t v = GetInt(params);
				return v != 0;
			}
			return true;
		}
		return false;
	}
	
public:
	struct Entry
	{
		std::string key, value;
	};
	std::string progName;
	std::vector<Entry> args;
	std::vector<std::string> floatingArgs;
};

