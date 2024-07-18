#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <bit>

namespace protocol{
	typedef struct Selection {
		std::map<std::string, std::vector<uint8_t>> Formats;
		bool operator==(const Selection&) const = default;
	} Selection;

	std::string Encode(const Selection& sel);

	Selection Decode(std::string data);
};
