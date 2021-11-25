#pragma once

#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING

#include <string>

// Define our Result object
struct Result {
	Result(const std::string& id,
		std::string contents,
		bool success);
	const std::string id;
	const std::string contents;
	const bool success;
};