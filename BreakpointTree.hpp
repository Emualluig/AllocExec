#ifndef _HEADER_BREAKPOINT_TREE_HPP_
#define _HEADER_BREAKPOINT_TREE_HPP_

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include "Byte.hpp"
#include "ExecutableMemory.hpp"

class BreakpointTree {
public:
	const BreakpointTree* left;
	const BreakpointTree* right;
	const std::size_t index;
	const float value;

	explicit BreakpointTree(float value, std::size_t index, BreakpointTree* left, BreakpointTree* right) :
		value{ value }, index{ index }, left{ left }, right{ right } {}
};

void tree_print(BreakpointTree const* node, uint32_t whitespace) {
	for (uint32_t i = 0; i < whitespace; i++) {
		std::cout << ' ';
	}
	std::cout << "Node(" << std::setprecision(2) << std::setw(6) << std::fixed << node->value << ", " << node->index << ")\n";
	if (node->left != nullptr) {
		tree_print(node->left, whitespace + 2);
	}
	else {
		for (uint32_t i = 0; i < whitespace + 2; i++) {
			std::cout << ' ';
		}
		std::cout << "nullptr\n";
	}
	if (node->right != nullptr) {
		tree_print(node->right, whitespace + 2);
	}
	else {
		for (uint32_t i = 0; i < whitespace + 2; i++) {
			std::cout << ' ';
		}
		std::cout << "nullptr\n";
	}
}

BreakpointTree* build_tree_impl(const std::vector<float>& intervals, std::size_t left, std::size_t right) {
	if (left >= right) {
		return nullptr;
	}
	else {
		auto breakpoint = left + (right - left) / 2;
		auto left_tree = build_tree_impl(intervals, left, breakpoint);
		auto right_tree = build_tree_impl(intervals, breakpoint + 1, right);

		return new BreakpointTree(intervals.at(breakpoint), breakpoint, left_tree, right_tree);
	}
}

BreakpointTree* build_tree(const std::vector<float>& intervals) {
	assert(intervals.size() >= 1);
	assert(intervals.size() <= INT32_MAX);
	return build_tree_impl(intervals, 0, intervals.size());
}

int32_t interval_search_linear(const std::vector<float>& intervals, const float value) noexcept {
	assert(intervals.size() >= 1);
	assert(intervals.size() <= INT32_MAX);
	for (std::size_t i = 0; i < intervals.size() - 1; i++) {
		if ((intervals.at(i) <= value) && (value < intervals.at(i + 1))) {
			return (int32_t)i;
		}
	}
	// The value is either below or above all intervals
	return -1;
}

int32_t interval_search_binary(const std::vector<float>& intervals, const float value) noexcept {
	assert(intervals.size() >= 1);
	assert(intervals.size() <= INT32_MAX);
	int32_t low = 0;
	int32_t high = intervals.size() - 1;
	while (low <= high) {
		int32_t mid = low + (high - low) / 2;
		if (mid + 1 >= intervals.size()) {
			// We have gone too far to the right, indicate that there is no valid interval
			return -1;
		} else if (intervals[mid] <= value && value < intervals[mid + 1]) {
			return mid;
		}
		else if (intervals[mid] < value) {
			low = mid + 1;
		}
		else {
			high = mid - 1;
		}
	}
	// We have gone too far to the left, indicate that there is no valid interval
	return -1;
}

namespace Codegen {
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
		uint32_t id;
		UsageType type;
	};

	void codegen_impl(
		BreakpointTree const* node,
		std::vector<Byte>& bytes,
		std::map<uint32_t, LabelDefinition>& label_definitions,
		std::vector<LabelUsage>& label_usages,
		std::vector<float>& numbers,
		ASM_context& context,
		uint32_t leftCount,
		uint32_t rightCount
	) {
		numbers.push_back(node->value);

		write_bytes(comiss_xmm0_DWORD_PTR_rip_PLUS_0x00000000, bytes);
		label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = (uint32_t)numbers.size() - 1, .type = UsageType::DEFERENCE });

		auto current_label = context.global_label_counter++;
		write_bytes(jae_0x00, bytes);
		label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = current_label, .type = UsageType::JUMP });

		if (node->left == nullptr && node->right == nullptr) {
			if (rightCount == 0) {
				// We are on the far left			
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
				// xmm0 < top->value
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(context.global_return_counter++, bytes);
				write_bytes(ret, bytes);
			}
		}
		else if (node->left == nullptr) {
			if (rightCount == 0) {
				// We are on the far left
				// xmm0 < top->value
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(-1, bytes);
				write_bytes(ret, bytes);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				codegen_impl(node->left, bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);
			}
			else {
				throw std::exception("This should not happen. There is an error in the binary tree's structure.");
			}
		}
		else if (node->right == nullptr) {
			if (leftCount == 0) {
				// We are on the far right
				// xmm0 < top->value
				codegen_impl(node->left, bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);

				// xmm0 >= top->value
				label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
				write_bytes(mov_eax_MISSING, bytes);
				write_bytes<uint32_t>(-1, bytes);
				write_bytes(ret, bytes);
			}
			else {
				throw std::exception("This should not happen. There is an error in the binary tree's structure.");
			}
		}
		else {
			// xmm0 < top->value
			codegen_impl(node->left, bytes, label_definitions, label_usages, numbers, context, leftCount + 1, rightCount);

			// xmm0 >= top->value
			label_definitions.insert({ current_label, LabelDefinition{.start_offset = bytes.size() } });
			codegen_impl(node->right, bytes, label_definitions, label_usages, numbers, context, leftCount, rightCount + 1);
		}
	}

	std::vector<Byte> codegen(const std::vector<float>& intervals, BreakpointTree const* root) {
		// C ABI calling convention: In this case we are passed "value" via xmm0 and we return into eax
		std::map<uint32_t, Codegen::LabelDefinition> label_defintions = {}; // label counter -> defintion
		std::vector<Codegen::LabelUsage> label_usages = {};
		Codegen::ASM_context context = { .global_label_counter = (uint32_t)intervals.size() + 10, .global_return_counter = 0 };

		std::vector<float> numbers = {};
		std::vector<Byte> bytes = {};
		Codegen::codegen_impl(root, bytes, label_defintions, label_usages, numbers, context, 0, 0);
		for (std::size_t i = 0; i < numbers.size(); i++) {
			auto length = write_bytes<float>(numbers.at(i), bytes);
			label_defintions.insert({
				i,
				Codegen::LabelDefinition{.start_offset = bytes.size() }
			});
		}
		Byte* ptr_no_offset = bytes.data();
		for (const auto& usage : label_usages) {
			Byte* bptr = ptr_no_offset + usage.start_offset;
			uint32_t* ptr = (uint32_t*)bptr;
			if (const auto iter = label_defintions.find(usage.id); iter != label_defintions.end()) {
				auto& def = iter->second;
				if (usage.type == Codegen::UsageType::DEFERENCE) {
					*ptr = def.start_offset - usage.start_offset - 8;
				}
				else if (usage.type == Codegen::UsageType::JUMP) {
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
};

class ExeIntervalSearch {
	using FUNC_PTR = int32_t(*)(float);
	void* memory;
public:
	explicit ExeIntervalSearch(const std::vector<float>& intervals) {
		auto tree = build_tree(intervals);
		tree_print(tree, 0);
		auto bytes = Codegen::codegen(intervals, tree);
		memory = exec_memory_create(bytes);
		delete tree;
	}
	~ExeIntervalSearch() {
		exec_memory_delete(memory);
	}
	int32_t run(float value) const noexcept {
		FUNC_PTR ptr = (FUNC_PTR)memory;
		return ptr(value);
	}
};

#endif // !_HEADER_BREAKPOINT_TREE_HPP_
