#include <vector>
#include <iostream>
#include <map>
#include <chrono>
#include <algorithm>
#include <fstream>
#include "BreakpointTree.hpp"
#include "JITable.hpp"
#include "Byte.hpp"
#include "ExecutableMemory.hpp"

#include <boost/unordered_map.hpp>
#include <gtl/phmap.hpp>

/* Could not get absl to compile
#include <absl/container/flat_hash_map.h>
*/

void test() {
	std::vector<float> x_values = {
		-3.0f, 0.0f, 1.0f, 3.0f, 4.0f, 5.0f
	};
	// Checking for correctness
	std::vector<float> checkpoints = {
		-3.1f,
		-3.0f,
		-2.9f,
		-0.1f,
		0.0f,
		0.1f,
		0.9f,
		1.0f,
		1.1f,
		2.9f,
		3.0f,
		3.1f,
		3.9f,
		4.0f,
		4.1f,
		4.9f,
		5.0f,
		5.1f,
	};

	auto jit = ExeIntervalSearch(x_values);

	for (const auto& v : checkpoints) {
		auto a = interval_search_linear(x_values, v);
		auto b = interval_search_binary(x_values, v);
		auto c = jit.run(v);
		std::cout
			<< a << ", "
			<< b << ", "
			<< c
			<< "\n";
	}
}

// Websites used to bootstrap ASM x64 generation process:
// https://godbolt.org/
// https://defuse.ca/online-x86-assembler.htm
// https://faydoc.tripod.com/cpu/jbe.htm
// https://www.felixcloutier.com/x86/comiss
// 
// Interesting things I found:
// https://andreasjhkarlsson.github.io/jekyll/update/2023/12/27/4-billion-if-statements.html
// https://sonictk.github.io/asm_tutorial/
// https://github.com/0xADE1A1DE/AssemblyLine/tree/main

int main_jitree() {
	std::vector<float> x_values = { 
		/*
		-100.0f, -3.0f, 0.0f, 1.0f, 3.0f, 4.0f, 5.0f, 
		6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 
		13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f
		*/
	};
	for (std::size_t i = 0; i < 1'000'000; i++) {
		x_values.push_back((float)i);
	}
	std::sort(x_values.begin(), x_values.end());

	auto t0 = std::chrono::high_resolution_clock::now();
	auto jit = ExeIntervalSearch(x_values);
	auto t1 = std::chrono::high_resolution_clock::now();

	// The intervals are [lower_bound, upper_bound)

	// Change margin and step to get longer/shorter runs
	float margin = 10.0f;
	int32_t steps = 99'999'999;
	float lower_bound = x_values.front() - margin;
	float upper_bound = x_values.back() + margin;
	float increment = (upper_bound - lower_bound) / steps;

	/*
	for (int32_t i = 0; i < steps; i++) {
		float f = increment * i + lower_bound;
		int32_t a = interval_search_linear(x_values, f);
		int32_t b = interval_search_binary(x_values, f);
		int32_t c = jit.run(f);
		std::cout << a << ", " << b << ", " << c << "\n";
	}
	*/
	
	bool disable_linear = true;
	bool disable_binary = false;
	bool disable_jit = false;
	bool print_step = false;
	int32_t radix = 100'000;

	// Use volatile to prevent being optimized away.
	auto t2 = std::chrono::high_resolution_clock::now();
	if (!disable_linear) {
		for (int32_t i = 0; i < steps; i++) {
			if (print_step && i % radix == 0) {
				std::cout << "Linear: (" << i + 1 << "/" << steps << ")\n";
			}
			float f = increment * i + lower_bound;
			volatile int32_t index = interval_search_linear(x_values, f);
		}
	}
	auto t3 = std::chrono::high_resolution_clock::now();
	if (!disable_binary) {
		for (int32_t i = 0; i < steps; i++) {
			if (print_step && i % radix == 0) {
				std::cout << "Binary: (" << i + 1 << "/" << steps << ")\n";
			}
			float f = increment * i + lower_bound;
			volatile int32_t index = interval_search_binary(x_values, f);
		}
	}
	auto t4 = std::chrono::high_resolution_clock::now();
	if (!disable_jit) {
		for (int32_t i = 0; i < steps; i++) {
			if (print_step && i % radix == 0) {
				std::cout << "JIT: (" << i + 1 << "/" << steps << ")\n";
			}
			float f = increment * i + lower_bound;
			volatile int32_t index = jit.run(f);
		}
	}
	auto t5 = std::chrono::high_resolution_clock::now();

	if (!disable_linear) {
		std::cout << "Linear Search: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2) << "\n";
	}
	if (!disable_binary) {
		std::cout << "Binary Search: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3) << "\n";
	}
	if (!disable_jit) {
		std::cout << "JIT: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4)
			<< ", compilation: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0) 
			<< " or " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0) << "\n";
	}
	
	return 0;
}

int main_jitable() {
	std::unordered_map<int32_t, void*> table_std = {};
	boost::unordered_map<int32_t, void*> table_boost = {};
	// absl::flat_hash_map<int32_t, void*> table_absl = {};
	gtl::flat_hash_map<int32_t, void*> table_gtl = {};

	int32_t start_point = -999'000'000;
	int32_t end_point = -start_point;
	int32_t build_step = 1'000;
	int32_t test_step = 1;
	for (int32_t i = start_point; i <= end_point; i += build_step) {
		if (i == 0) {
			continue;
		}
		table_std.insert({ i, (void*)i });
		table_boost.insert({ i, (void*)i });
		// table_absl.insert({ i, (void*)i });
		table_gtl.insert({ i, (void*)i });
	}

	auto t0 = std::chrono::high_resolution_clock::now();
	auto jit_v1 = ExeTable(table_std);
	auto t1 = std::chrono::high_resolution_clock::now();

	auto t2 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = table_std.find(i) != table_std.end();
	}
	auto t3 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = jit_v1.run(i);
	}
	auto t4 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = table_boost.find(i) != table_boost.end();
	}
	auto t5 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = table_gtl.find(i) != table_gtl.end();
	}
	auto t6 = std::chrono::high_resolution_clock::now();

	std::cout
		<< "Interval: [" << start_point << ", " << end_point << "], step ratio: " << test_step << "/" << build_step << ", entries: " << table_std.size() << "\n"
		<< "unordered_map: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2) << "\n"
		<< "jit: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3)
		<< ", compilation: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0) 
		<< ", total: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3 + t1 - t0) << "\n"
		<< "boost: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4) << "\n"
		<< "gtl: " << std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5) << "\n";

	return 0;
}

void test_jitable() {
	std::unordered_map<int32_t, void*> table_std = {};

	for (int32_t i = -1'000; i <= 1'000; i += 1) {
		if (i == 0 || i % 2 == 0) {
			continue;
		}
		table_std.insert({ i, (void*)i });
	}

	auto t0 = std::chrono::high_resolution_clock::now();
	auto jit_v1 = ExeTable(table_std, false);
	auto t1 = std::chrono::high_resolution_clock::now();
	auto jit_v2 = ExeTable(table_std, true);
	auto t2 = std::chrono::high_resolution_clock::now();

	for (int32_t i = -100; i <= 100; i += 1) {
		auto has_in_std = table_std.find(i) != table_std.end();
		auto has_in_jit_v1 = jit_v1.run(i) != 0;
		auto has_in_jit_v2 = jit_v2.run(i) != 0;
		std::cout << "i: " << std::setw(5) << i 
			<< ", std: " << std::boolalpha << std::setw(5) << has_in_std 
			<< ", jit_v1: " << std::setw(5) << has_in_jit_v1
			<< ", jit_v2: " << std::setw(5) << has_in_jit_v2 << "\n";
	}

	std::cout << "JIT v1: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0) << ", size: " << jit_v1.size << "\n";
	std::cout << "JIT v2: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1) << ", size: " << jit_v2.size << "\n";

	/*
	{
		std::ofstream fout;
		fout.open("v1.bin", std::ios::binary | std::ios::out);

		fout.write((char*)jit_v1.memory, jit_v1.size);

		fout.close();
	}
	{
		std::ofstream fout;
		fout.open("v2.bin", std::ios::binary | std::ios::out);

		fout.write((char*)jit_v2.memory, jit_v2.size);

		fout.close();
	}
	*/
}

void jitable_version_compare() {
	std::unordered_map<int32_t, void*> table_std = {};

	int32_t start_point = -1'000'000;
	int32_t end_point = -start_point;
	int32_t build_step = 1'00;
	int32_t test_step = 1;
	for (int32_t i = start_point; i <= end_point; i += build_step) {
		if (i == 0) {
			continue;
		}
		table_std.insert({ i, (void*)i });
	}

	auto t0 = std::chrono::high_resolution_clock::now();
	auto jit_v1 = ExeTable(table_std);
	auto t1 = std::chrono::high_resolution_clock::now();
	auto jit_v2 = ExeTable(table_std, true);
	auto t2 = std::chrono::high_resolution_clock::now();


	auto t3 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = jit_v1.run(i);
	}
	auto t4 = std::chrono::high_resolution_clock::now();
	for (int32_t i = start_point; i <= end_point; i += test_step) {
		volatile bool index = jit_v2.run(i);
	}
	auto t5 = std::chrono::high_resolution_clock::now();

	std::cout
		<< "jit v1: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3)
		<< ", compilation: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
		<< ", total: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3 + t1 - t0) << "\n";
	std::cout
		<< "jit v2: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4)
		<< ", compilation: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
		<< ", total: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4 + t2 - t1) << "\n";
}

int main() {
	// main_jitree();
	// main_jitable();

	test_jitable();
	// jitable_version_compare();


	return 0;
}