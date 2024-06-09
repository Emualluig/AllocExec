#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <array>
#include <map>
#include <iomanip>
#include <stack>
#include <cassert>

using Byte = unsigned char;
template<typename T> concept isTriviallyCopyable = std::is_trivially_copyable<T>::value;
template <typename Object> requires isTriviallyCopyable<Object>
std::size_t write_bytes(Object source, std::vector<Byte>& destination) noexcept {
	constexpr std::size_t size = sizeof(Object);
	const Byte* ptr = (const Byte*)&source;
	for (std::size_t i = 0; i < size; i++) {
		destination.push_back(ptr[i]);
	}
	return size;
}
std::size_t write_bytes(const std::vector<Byte>& source, std::vector<Byte>& destination) noexcept {
	for (std::size_t i = 0; i < source.size(); i++) {
		destination.push_back(source.at(i));
	}
	return source.size();
}

const std::vector<Byte> comiss_xmm0_DWORD_PTR_rip_PLUS_0x00 = {
	0x0F, 0x2F, 0x05, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> jae_0x00 = {
	0x0F, 0x83, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> jbe_0x00 = {
	0x0F, 0x86, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> mov_eax_MISSING = {
	0xB8
};
const std::vector<Byte> ret = {
	0xC3
};
const std::vector<Byte> movss_xmm1_DWORD_PTR_rip_PLUS_0x00 = {
	0xF3, 0x0F, 0x10, 0x0D, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> comiss_xmm0_xmm1 = {
	0x0F, 0x2F, 0xC1
};


void pretty_print(const std::span<float> vec) {
	std::cout << "[ ";
	for (std::size_t i = 0; i < vec.size(); i++) {
		if (i + 1 == vec.size()) {
			std::cout << vec[i];
		}
		else {
			std::cout << vec[i] << ", ";
		}
	}
	std::cout << " ]\n";
}

// https://andreasjhkarlsson.github.io/jekyll/update/2023/12/27/4-billion-if-statements.html
// https://sonictk.github.io/asm_tutorial/
// https://github.com/0xADE1A1DE/AssemblyLine/tree/main

class BreakpointTree {
public:
	float value;
	int64_t index;
	BreakpointTree* left;
	BreakpointTree* right;

	explicit BreakpointTree(float value, int64_t interval_id) : value{ value }, index{ interval_id }, left { nullptr }, right{ nullptr } {}
	explicit BreakpointTree(float value, int64_t interval_id, BreakpointTree* left, BreakpointTree* right) : value{ value }, index{ interval_id }, left{ left }, right{ right } {}
	~BreakpointTree() {
		delete left;
		delete right;
	}

	struct ASM_context {
		uint32_t global_label_counter;
		uint32_t global_return_counter;
	};

	struct LabelDefinition {
		std::size_t start_offset;
	};
	enum struct UsageType {
		JUMP,
		DEFERENCE
	};
	struct LabelUsage {
		std::size_t start_offset;
		std::size_t length;
		uint32_t id;
		UsageType type;
	};
private:
	void codegen_bin_recur(
		std::vector<Byte>& bytes,
		std::map<uint32_t, LabelDefinition>& label_definitions,
		std::vector<LabelUsage>& label_usages,
		std::vector<float>& numbers,
		ASM_context& context,
		uint32_t leftCount,
		uint32_t rightCount
	) {
		numbers.push_back(value);

		/*
		write_bytes(movss_xmm1_DWORD_PTR_rip_PLUS_0x00, bytes);
		label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = (uint32_t)numbers.size() - 1, .type = UsageType::DEFERENCE });
		write_bytes(comiss_xmm0_xmm1, bytes);
		*/

		write_bytes(comiss_xmm0_DWORD_PTR_rip_PLUS_0x00, bytes);
		label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = (uint32_t)numbers.size() - 1, .type = UsageType::DEFERENCE });


		if (left == nullptr && right == nullptr) {
			if (rightCount == 0) {
				// We are on the far left			
				auto current_label = context.global_label_counter++;
				write_bytes(jae_0x00, bytes);
				label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

				// xmm0 < top->value
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<int32_t>(-1, bytes);
				write_bytes(ret, bytes);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);
			}
			else if (leftCount == 0) {
				// We are on the far right
				auto current_label = context.global_label_counter++;
				write_bytes(jae_0x00, bytes);
				label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

				// xmm0 < top->value
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<int32_t>(-1, bytes);
				write_bytes(ret, bytes);
			}
			else {
				auto current_label = context.global_label_counter++;
				write_bytes(jae_0x00, bytes);
				label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

				// xmm0 < top->value
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{ .start_offset = bytes.size() }});
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);
			}
		}
		else if (left == nullptr) {
			/*
			if (rightCount == 0) {
				// We are on the far left
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, -1\n";
				ss << "ret\n";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				right->codegen_asm(ss, context, leftCount, rightCount + 1);
			}
			else {
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
			}
			*/
		}
		else if (right == nullptr) {
			/*
			if (leftCount == 0) {
				// We are on the far right
				auto current_label = context.global_label_counter++;
				write_bytes(jae_0x00, bytes);
				label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

				// xmm0 < top->value
				left->codegen_bin_recur(bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(-1, bytes);
				write_bytes(ret, bytes);
			}
			else {
				auto current_label = context.global_label_counter++;
				write_bytes(jae_0x00, bytes);
				label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

				// xmm0 < top->value
				left->codegen_bin_recur(bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);
			}
			*/
		}
		else {
			auto current_label = context.global_label_counter++;
			write_bytes(jae_0x00, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .length = 4, .id = current_label, .type = UsageType::JUMP });

			// xmm0 < top->value
			left->codegen_bin_recur(bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);

			// xmm0 >= top->value
			label_definitions.insert({ current_label, LabelDefinition{ .start_offset = bytes.size() }});
			right->codegen_bin_recur(bytes, label_definitions, label_usages, numbers, context, leftCount, rightCount + 1);
		}
	}
public:

	std::vector<Byte> codegen_bin(const std::vector<float>& values) {
		std::map<uint32_t, LabelDefinition> label_defintions = {}; // label counter -> defintion
		std::vector<LabelUsage> label_usages = {};
		ASM_context context = { .global_label_counter = (uint32_t)values.size() + 10, .global_return_counter = 0 };
		
		std::vector<float> numbers = {};
		std::vector<Byte> bytes = {};
		codegen_bin_recur(bytes, label_defintions, label_usages, numbers, context, 0, 0);
		for (std::size_t i = 0; i < numbers.size(); i++) {
			auto length = write_bytes<float>(numbers.at(i), bytes);
			label_defintions.insert({
				i,
				LabelDefinition{ .start_offset = bytes.size() }
			});
		}
		Byte* ptr_no_offset = bytes.data();
		for (const auto& usage : label_usages) {
			Byte* bptr = ptr_no_offset + usage.start_offset;
			uint32_t* ptr = (uint32_t*)bptr;
			if (const auto iter = label_defintions.find(usage.id); iter != label_defintions.end()) {
				auto& def = iter->second;
				if (usage.type == UsageType::DEFERENCE) {
					*ptr = def.start_offset - usage.start_offset - 8;
				}
				else if (usage.type == UsageType::JUMP) {
					*ptr = def.start_offset - usage.start_offset - 4;
				}
				else {
					throw std::exception("Unknown usage type received.");
				}
			}
			else {
				throw std::exception("Could not find label definition.");
			}
		}

		return bytes;
	}

	void codegen_asm(
		std::stringstream& ss, 
		ASM_context& context,
		uint32_t leftCount,
		uint32_t rightCount
	) const {
		if (left == nullptr && right == nullptr) {
			if (rightCount == 0) {
				// We are on the far left
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, -1\n";
				ss << "ret\n";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
			}
			else if (leftCount == 0) {
				// We are on the far right
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, -1\n";
				ss << "ret\n";
			}
			else {
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
			}
		}
		else if (left == nullptr) {
			if (rightCount == 0) {
				// We are on the far left
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, -1\n";
				ss << "ret\n";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				right->codegen_asm(ss, context, leftCount, rightCount + 1);
			}
			else {
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
			}
		}
		else if (right == nullptr) {
			if (leftCount == 0) {
				// We are on the far right
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				left->codegen_asm(ss, context, leftCount + 1, rightCount);
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, -1\n";
				ss << "ret\n";
			}
			else {
				ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
				auto current_label = context.global_label_counter++;
				ss << "jbe .L" << current_label << "\n";
				// xmm0 < top->value
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret";
				// xmm0 >= top->value
				ss << ".L" << current_label << "\n";
				ss << "mov eax, " << context.global_return_counter++ << "\n";
				ss << "ret\n";
			}
		}
		else {
			ss << "comiss xmm0, CONSTANT_(" << value << ")\n";
			auto current_label = context.global_label_counter++;
			ss << "jbe .L" << current_label << "\n";
			// xmm0 < top->value
			left->codegen_asm(ss, context, leftCount + 1, rightCount);
			// xmm0 >= top->value
			ss << ".L" << current_label << "\n";
			right->codegen_asm(ss, context, leftCount, rightCount + 1);
		}
	}

	void codegen_text(std::stringstream& ss, int leftCount, int rightCount, int64_t& id) const {
		if (left == nullptr && right == nullptr) {
			if (rightCount == 0) {
				// We are on the far left
				ss << "if (x < " << value << ") {\n";
				ss << "return -1;\n";
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return " << id << ";\n";
				ss << "}\n";
				id += 1;
			}
			else if (leftCount == 0) {
				// We are on the far right
				ss << "if (x < " << value << ") {\n";
				ss << "return " << id << ";\n";
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return -1;\n";
				ss << "}\n";
				id += 1;
			}
			else {
				ss << "if (x < " << value << ") {\n";
				ss << "return " << id << ";\n";
				id += 1;
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return " << id << ";\n";
				ss << "}\n";
				id += 1;
			}
		}
		else if (left == nullptr) {
			if (rightCount == 0) {
				// We are on the far left
				ss << "if (x < " << value << ") {\n";
				ss << "return -1;\n";
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				right->codegen_text(ss, leftCount, rightCount + 1, id);
				ss << "}\n";
			}
			else {
				ss << "// left == nullptr && rightCount != 0\n";
				ss << "if (x < " << value << ") {\n";
				ss << "return " << id << ";\n";
				id += 1;
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return " << id << ";\n";
				ss << "}\n";
				id += 1;
			}
		}
		else if (right == nullptr) {
			if (leftCount == 0) {
				// We are on the far right
				ss << "if (x < " << value << ") {\n";
				left->codegen_text(ss, leftCount + 1, rightCount, id);
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return -1;\n";
				ss << "}\n";
			}
			else {
				ss << "// right == nullptr && leftCount != 0\n";
				ss << "if (x < " << value << ") {\n";
				ss << "return " << id << ";\n";
				id += 1;
				ss << "} else {\n";
				ss << "// x >= " << value << "\n";
				ss << "return " << id << ";\n";
				ss << "}\n";
				id += 1;
			}
		}
		else {
			ss << "if (x < " << value << ") {\n";
			left->codegen_text(ss, leftCount + 1, rightCount, id);
			ss << "} else {\n";
			ss << "// x >= " << value << "\n";
			right->codegen_text(ss, leftCount, rightCount + 1, id);
			ss << "}\n";
		}
	}
};

BreakpointTree* build_breakpoint_tree(std::span<float> values, int64_t& index) {
	if (values.size() == 1) {
		index += 1;
		return new BreakpointTree(values[0], index - 1);
	}
	else if (values.size() == 0) {
		return nullptr;
	}
	else {
		auto breakpoint_index = values.size() / 2;
		auto left = values.subspan(0, breakpoint_index);
		auto right = values.subspan(breakpoint_index + 1, values.size() - (breakpoint_index + 1));

		auto left_result = build_breakpoint_tree(left, index);
		auto right_result = build_breakpoint_tree(right, index);

		index += 1;
		return new BreakpointTree(values[breakpoint_index], index - 1, left_result, right_result);
	}
}

#include <windows.h>

void* exec_memory_create(const std::span<const Byte> memory) {
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
	auto span = std::span<Byte>(machine_code);
		
	using FUNC_PTR = float (*)(float, float*);
	auto memory = exec_memory_create(span);

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
	auto span = std::span<Byte>(machine_code);

	using FUNC_PTR = uint32_t(*)(float);
	auto memory = exec_memory_create(span);

	float* multipliers = nullptr;
	FUNC_PTR ptr = (FUNC_PTR)memory;
	auto result = ptr(x);

	exec_memory_delete(memory);
	return result;
}

int main() {
	std::vector<float> x_values = { -3.0f, 0.0f, 1.0f, 3.0f, 4.0f, 5.0f };
	// std::vector<float> x_values = { -3.0f, 0.0f, 1.0f };
	std::sort(x_values.begin(), x_values.end());

	int64_t build_id = 0;
	auto tree = build_breakpoint_tree(x_values, build_id);

	std::cout << asm_mult_test(129.0f, 6.0f) << "\n";
	
	/*
	auto result = asm_is_x_greater_than_or_equal_test(100.0f, 100.0f);
	std::cout << std::boolalpha << result << std::endl;
	*/

	int64_t print_id = 0;
	std::stringstream ss;
	tree->codegen_text(ss, 0, 0, print_id);
	std::cout << "=========\n" << ss.str() << "=========\n";

#if false
	auto bytes = tree->codegen_bin(x_values);
	using FUNC_PTR = int32_t(*)(float);
	auto memory = exec_memory_create(bytes);
	FUNC_PTR ptr = (FUNC_PTR)memory;
	auto result = ptr(-3.0f);

	assert(ptr(-3.5f) == -1);
	assert(ptr(-2.9f) == 0);
	assert(ptr(-0.1f) == 0);
	assert(ptr(0.1f) == 1);
	assert(ptr(0.9f) == 1);
	assert(ptr(1.1f) == -1);

	exec_memory_delete(memory);
#endif
	
#if false
	for (const auto byte : bytes) {
		std::cout << std::hex << (uint32_t)byte;
	}
	std::cout << std::endl;
#endif

	return 0;
}