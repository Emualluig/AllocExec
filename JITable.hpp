#ifndef _HEADER_JITABLE_HPP_
#define _HEADER_JITABLE_HPP_

#include <utility>
#include <unordered_map>
#include <map>
#include <cassert>
#include <algorithm>
#include <limits>
#include "Byte.hpp"

class JITableTree {
public:
	const int32_t key;
	const void* value;
	const JITableTree* left;
	const JITableTree* right;

	explicit JITableTree(const int32_t key, const void* value, const JITableTree* left, const JITableTree* right) : key{ key }, value{ value }, left{ left }, right{ right } {}

	~JITableTree() {
		if (left != nullptr) {
			delete left;
		}
		if (right != nullptr) {
			delete right;
		}
	}

	static const int32_t NOT_FOUND = std::numeric_limits<int32_t>::min();
};

JITableTree* build_table_tree_impl(const std::vector<std::pair<int32_t, void*>>& kv_pairs, std::size_t left, std::size_t right) {
	if (left >= right) {
		return nullptr;
	}
	else {
		auto breakpoint = left + (right - left) / 2;
		auto left_tree = build_table_tree_impl(kv_pairs, left, breakpoint);
		auto right_tree = build_table_tree_impl(kv_pairs, breakpoint + 1, right);
		const auto& kv = kv_pairs.at(breakpoint);
		return new JITableTree(kv.first, kv.second, left_tree, right_tree);
	}
}

JITableTree* build_table_tree(const std::unordered_map<int32_t, void*>& basic_table) {
	std::vector<std::pair<int32_t, void*>> kv_pairs = {};
	kv_pairs.reserve(basic_table.size() + 10);
	for (const auto& kv_map : basic_table) {
		if (kv_map.second == nullptr) {
			throw std::exception("Not allowed to have a value be nullptr");
		}
		kv_pairs.push_back(kv_map);
	}
	std::sort(kv_pairs.begin(), kv_pairs.end());
	return build_table_tree_impl(kv_pairs, 0, kv_pairs.size());
}

namespace CodegenTable {
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
		JITableTree const* node,
		std::vector<Byte>& bytes,
		std::map<uint32_t, LabelDefinition>& label_definitions,
		std::vector<LabelUsage>& label_usages,
		ASM_context& context,
		uint32_t leftCount,
		uint32_t rightCount
	) {
		if (node->left == nullptr && node->right == nullptr) {
			write_bytes(cmp_ecx_MISSING_4_BYTES, bytes);
			write_bytes(node->key, bytes);

			auto first_jump_label = context.global_label_counter++;
			write_bytes(jne_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = first_jump_label, .type = UsageType::JUMP });

			// edi == node->key
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes(node->value, bytes);
			write_bytes(ret, bytes);

			// edi != node->key
			label_definitions.insert({ first_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes<void*>(nullptr, bytes);
			write_bytes(ret, bytes);
		}
		else if (node->left == nullptr) {
			write_bytes(cmp_ecx_MISSING_4_BYTES, bytes);
			write_bytes(node->key, bytes);

			auto first_jump_label = context.global_label_counter++;
			write_bytes(jge_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = first_jump_label, .type = UsageType::JUMP });

			// edi < node->key, nothing exists here
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes<void*>(nullptr, bytes);
			write_bytes(ret, bytes);

			// edi >= node->key
			label_definitions.insert({ first_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			auto second_jump_label = context.global_label_counter++;
			write_bytes(jne_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = second_jump_label, .type = UsageType::JUMP });

			// edi == node->key
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes(node->value, bytes);
			write_bytes(ret, bytes);

			// edi > node->key
			label_definitions.insert({ second_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			codegen_impl(node->right, bytes, label_definitions, label_usages, context, leftCount, rightCount + 1);
		}
		else if (node->right == nullptr) {
			write_bytes(cmp_ecx_MISSING_4_BYTES, bytes);
			write_bytes(node->key, bytes);

			auto first_jump_label = context.global_label_counter++;
			write_bytes(jge_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = first_jump_label, .type = UsageType::JUMP });

			// edi < node->key
			codegen_impl(node->left, bytes, label_definitions, label_usages, context, leftCount + 1, rightCount);

			// edi >= node->key
			label_definitions.insert({ first_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			auto second_jump_label = context.global_label_counter++;
			write_bytes(jne_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = second_jump_label, .type = UsageType::JUMP });

			// edi == node->key
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes(node->value, bytes);
			write_bytes(ret, bytes);

			// edi > node->key, there is nothing here
			label_definitions.insert({ second_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes<void*>(nullptr, bytes);
			write_bytes(ret, bytes);
		}
		else {
			// We receive the argument through ecx
			write_bytes(cmp_ecx_MISSING_4_BYTES, bytes);
			write_bytes(node->key, bytes);
			
			auto first_jump_label = context.global_label_counter++;
			write_bytes(jge_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = first_jump_label, .type = UsageType::JUMP });

			// edi < node->key
			codegen_impl(node->left, bytes, label_definitions, label_usages, context, leftCount + 1, rightCount);

			// edi >= node->key
			label_definitions.insert({ first_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			auto second_jump_label = context.global_label_counter++;
			write_bytes(jne_0x00000000, bytes);
			label_usages.push_back(LabelUsage{ .start_offset = bytes.size() - 4, .id = second_jump_label, .type = UsageType::JUMP });

			// edi == node->key
			write_bytes(mov_rax_MISSING_8_BYTES, bytes);
			write_bytes(node->value, bytes);
			write_bytes(ret, bytes);

			// edi > node->key
			label_definitions.insert({ second_jump_label, LabelDefinition{.start_offset = bytes.size() } });
			codegen_impl(node->right, bytes, label_definitions, label_usages, context, leftCount, rightCount + 1);
		}
	}

	std::vector<Byte> codegen(JITableTree const* root) {
		std::map<uint32_t, LabelDefinition> label_defintions = {}; // label counter -> defintion
		std::vector<LabelUsage> label_usages = {};
		ASM_context context = { .global_label_counter = 0, .global_return_counter = 0 };

		std::vector<Byte> bytes = {};
		codegen_impl(root, bytes, label_defintions, label_usages, context, 0, 0);
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
};

class ExeTable {
	using FUNC_PTR = uint64_t(*)(int32_t);
	void* memory;
public:
	explicit ExeTable(const std::unordered_map<int32_t, void*>& basic_table) {
		auto tree = build_table_tree(basic_table);
		auto bytes = CodegenTable::codegen(tree);
		memory = exec_memory_create(bytes);
		delete tree;
	}
	~ExeTable() {
		exec_memory_delete(memory);
	}
	uint64_t run(int32_t key) const noexcept {
		FUNC_PTR ptr = (FUNC_PTR)memory;
		return ptr(key);
	}
};

#endif // !_HEADER_JITABLE_HPP_
