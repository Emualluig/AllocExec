#ifndef _HEADER_EXECUTABLE_MEMORY_HPP_
#define _HEADER_EXECUTABLE_MEMORY_HPP_

#include <windows.h>
#include <vector>
#include "Byte.hpp"

void* exec_memory_create(const std::vector<Byte>& memory) {
	// Taken from: https://stackoverflow.com/questions/40936534/how-to-alloc-a-executable-memory-buffer
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	const auto page_size = system_info.dwPageSize;
	const auto allocation_size = (memory.size() / page_size + 1) * page_size;

	// prepare the memory in which the machine code will be put (it's not executable yet):
	auto buffer = VirtualAlloc(nullptr, allocation_size, MEM_COMMIT, PAGE_READWRITE);
	if (buffer == nullptr) {
		return nullptr;
	}

	// copy the machine code into that memory:
	std::memcpy(buffer, memory.data(), memory.size());

	// mark the memory as executable:
	DWORD dummy;
	VirtualProtect(buffer, memory.size(), PAGE_EXECUTE_READ, &dummy);

	return buffer;
}

bool exec_memory_delete(void* memory) {
	return VirtualFree(memory, 0, MEM_RELEASE);
}

float asm_mult_test(float multiplicand, float multiplier) {
	auto machine_code = std::vector<Byte>{
		0xf3, 0x0f, 0x59, 0x05, 0x01, 0x00, 0x00, 0x00, // mulss xmm0, DWORD PTR [rip + 0x1]
		0xc3 // ret
	};
	write_bytes<float>(multiplier, machine_code);

	using FUNC_PTR = float (*)(float, float*);
	auto memory = exec_memory_create(machine_code);

	float* multipliers = nullptr;
	FUNC_PTR ptr = (FUNC_PTR)memory;
	auto result = ptr(multiplicand, multipliers);

	exec_memory_delete(memory);
	return result;
}

bool asm_is_x_greater_than_or_equal_test(float x, float value) {
	auto machine_code = std::vector<Byte>{
		0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, 0
		0x0F, 0x2F, 0x05, 0x01, 0x00, 0x00, 0x00, // comiss xmm0, DWORD PTR [rip+0x18]
		0x0F, 0x86, 0x05, 0x00, 0x00, 0x00, // jbe 17
		0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1 (if it is greater or equal to rip+0x1 it gets here)
		0xc3, // ret
	};
	write_bytes<float>(value, machine_code);

	using FUNC_PTR = uint32_t(*)(float);
	auto memory = exec_memory_create(machine_code);

	float* multipliers = nullptr;
	FUNC_PTR ptr = (FUNC_PTR)memory;
	auto result = ptr(x);

	exec_memory_delete(memory);
	return result;
}


#endif // !_HEADER_EXECUTABLE_MEMORY_HPP_
