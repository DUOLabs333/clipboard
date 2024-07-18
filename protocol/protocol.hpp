#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace protocol{
	typedef struct {
		std::map<std::string, std::vector<uint8_t>> Formats;
	} Selection;

	std::string Encode(Selection sel);

	Selection Decode(std::string data);
};
