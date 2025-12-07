#pragma once
#include <string>
#include <stdexcept>
namespace StringUtils{
	enum StringComparison {
		OrdinalIgnoreCase,
		Ordinal
	};
	bool Equals(std::string self, std::string other, StringComparison comparison = Ordinal) {
		if (self.length() != other.length()) {
			return false;
		}
		switch (comparison)
		{
		case StringUtils::OrdinalIgnoreCase:
			for (size_t i = 0; i < self.length(); ++i) {
				if (std::tolower(static_cast<unsigned char>(self[i])) !=
					std::tolower(static_cast<unsigned char>(other[i]))) {
					return false;
				}
			}
			break;
		case StringUtils::Ordinal:
			for (size_t i = 0; i < self.length(); ++i) {
				if (static_cast<unsigned char>(self[i]) !=
					static_cast<unsigned char>(other[i])) {
					return false;
				}
			}
			break;
		default:
			break;
		}
		return true;
	}

	std::string ToLower(std::string str) {
		for (size_t i = 0; i < str.length(); ++i) {
			str[i] = std::tolower(static_cast<unsigned char>(str[i]));}
		return str;
	}

	std::string ToUpper(std::string str) {
		for (size_t i = 0; i < str.length(); ++i) {
			str[i] = std::toupper(static_cast<unsigned char>(str[i]));}
		return str;
	}
}
