#include <vector>
#include <iostream>
#include <map>
#include <chrono>
#include <algorithm>
#include "BreakpointTree.hpp"
#include "Byte.hpp"
#include "ExecutableMemory.hpp"

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

int main() {
	std::vector<float> x_values = { 
		/*
		-100.0f, -3.0f, 0.0f, 1.0f, 3.0f, 4.0f, 5.0f, 
		6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 
		13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f
		*/
	};
	for (std::size_t i = 0; i < 1'000; i++) {
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
	
	// Use volatile to prevent being optimized away.
	auto t2 = std::chrono::high_resolution_clock::now();
	for (int32_t i = 0; i < steps; i++) {
		float f = increment * i + lower_bound;
		volatile int32_t index = interval_search_linear(x_values, f);
	}
	auto t3 = std::chrono::high_resolution_clock::now();
	for (int32_t i = 0; i < steps; i++) {
		float f = increment * i + lower_bound;
		volatile int32_t index = interval_search_binary(x_values, f);
	}
	auto t4 = std::chrono::high_resolution_clock::now();
	for (int32_t i = 0; i < steps; i++) {
		float f = increment * i + lower_bound;
		volatile int32_t index = jit.run(f);
	}
	auto t5 = std::chrono::high_resolution_clock::now();

	std::cout << "Linear Search: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2) << "\n";
	std::cout << "Binary Search: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3) << "\n";
	std::cout << "JIT: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4) 
				<< ", compilation: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0) << "\n";
	
	return 0;
}