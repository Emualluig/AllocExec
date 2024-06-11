#ifndef _HEADER_BYTE_HPP_
#define _HEADER_BYTE_HPP_

#include <vector>

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

// It is missing 4 bytes for the constant
const std::vector<Byte> mov_eax_MISSING_4_BYTES = {
	0xB8
};
const std::vector<Byte> mov_rax_MISSING_8_BYTES = {
	0x48, 0xB8
};
const std::vector<Byte> comiss_xmm0_DWORD_PTR_rip_PLUS_0x00000000 = {
	0x0F, 0x2F, 0x05, 0x00, 0x00, 0x00, 0x00
};
// Often used for floating point comparaison
const std::vector<Byte> jae_0x00000000 = {
	0x0F, 0x83, 0x00, 0x00, 0x00, 0x00
};
// Often used for integer comparaison
const std::vector<Byte> jge_0x00000000 = {
	0x0F, 0x8D, 0x00, 0x00, 0x00, 0x00 // This is jge, not jae
};
const std::vector<Byte> jne_0x00000000 = {
	0x0F, 0x85, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> jbe_0x00000000 = {
	0x0F, 0x86, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> ret = {
	0xC3
};
const std::vector<Byte> movss_xmm1_DWORD_PTR_rip_PLUS_0x00000000 = {
	0xF3, 0x0F, 0x10, 0x0D, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> comiss_xmm0_xmm1 = {
	0x0F, 0x2F, 0xC1
};
const std::vector<Byte> cmp_edi_0x00000000 = {
	0x81, 0xFF, 0x00, 0x00, 0x00, 0x00
};
const std::vector<Byte> cmp_ecx_MISSING_4_BYTES = {
	0x81, 0xF9
};
const std::vector<Byte> cmp_edi_MISSING_2_BYTES = {
	0x82, 0xFF
};
const std::vector<Byte> cmp_edi_MISSING_1_BYTE = {
	0x83, 0xFF, 0x00
};

#endif // !_HEADER_BYTE_HPP_
