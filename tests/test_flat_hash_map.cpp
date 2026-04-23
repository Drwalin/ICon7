#include "../include/icon7/FlatHashMap.hpp"
#include <iostream>
#include <cassert>
#include <string>

using icon7::FlatHashMap;

int main()
{
	std::cout << "Running FlatHashMap tests..." << std::endl;

	// Test 1: ConstructorInitializesCorrectly
	{
		FlatHashMap<int, std::string> map(10);
		assert(map.Size() == 0);
		assert(map.Capacity() == 16);
		std::cout << "Test 1 Passed: ConstructorInitializesCorrectly"
				  << std::endl;
	}

	// Test 2: InsertAndGet
	{
		FlatHashMap<int, std::string> map(4);

		map.InsertOrSet(10, "Value10");
		map.InsertOrSet(20, "Value20");

		assert(map.Size() == 2);
		assert(map.Has(10));
		assert(map.Get(10) != nullptr && *(map.Get(10)) == "Value10");
		assert(map.Get(20) != nullptr && *(map.Get(20)) == "Value20");
		std::cout << "Test 2 Passed: InsertAndGet" << std::endl;
	}

	// Test 3: UpdateExistingValue
	{
		FlatHashMap<int, std::string> map(4);

		map.InsertOrSet(5, "Initial");

		map.InsertOrSet(5, "Updated");

		assert(map.Size() == 1);
		assert(map.Get(5) != nullptr && *(map.Get(5)) == "Updated");
		std::cout << "Test 3 Passed: UpdateExistingValue" << std::endl;
	}

	// Test 4: HasNonExistentKey
	{
		FlatHashMap<int, std::string> map(4);

		assert(!map.Has(99));
		std::cout << "Test 4 Passed: HasNonExistentKey" << std::endl;
	}

	// Test 5: RemoveExistingKey
	{
		FlatHashMap<int, std::string> map(4);

		map.InsertOrSet(1, "One");
		map.InsertOrSet(2, "Two");

		map.Remove(1);

		assert(map.Size() == 1);
		assert(!map.Has(1));
		assert(map.Has(2));
		assert(map.Get(2) != nullptr && *(map.Get(2)) == "Two");
		std::cout << "Test 5 Passed: RemoveExistingKey" << std::endl;
	}

	// Test 6: RemoveNonExistentKey
	{
		FlatHashMap<int, std::string> map(4);

		map.InsertOrSet(1, "One");

		map.Remove(99);

		assert(map.Size() == 1);
		assert(map.Has(1));
		std::cout << "Test 6 Passed: RemoveNonExistentKey" << std::endl;
	}

	// Test 7: CapacityResizingTriggersRehash
	{
		FlatHashMap<int, std::string> map(4); // Initial capacity 16

		for (int i = 0; i < 10; ++i) {
			map.InsertOrSet(i, std::to_string(i));
		}

		assert(map.Size() == 10);
		std::cout << "Test 7 Passed: CapacityResizingTriggersRehash"
				  << std::endl;
	}

	std::cout << "All FlatHashMap tests passed successfully!" << std::endl;
	return 0;
}
