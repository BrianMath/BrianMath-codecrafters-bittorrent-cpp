#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <variant>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(std::string& encoded_value);

json decode_string(std::string& encoded_value) {
	size_t colon_index = encoded_value.find(':');

	if (colon_index != std::string::npos) {
		std::string number_string = encoded_value.substr(0, colon_index);
		int64_t number = std::stoll(number_string);
		std::string str = encoded_value.substr(colon_index + 1, number);

		int64_t total_number = number + colon_index + 1;
		encoded_value = encoded_value.substr(total_number);
		
		return json(str);
	}
	
	throw std::runtime_error("Invalid encoded string: missing colon > " + encoded_value);
}

json decode_integer(std::string& encoded_value) {
	size_t len = encoded_value.length();
	if (len <= 2) {
		throw std::runtime_error("Invalid encoded integer: too short > " + encoded_value);
	}
	
	size_t index_end = 0;

	// Handle invalid numbers, like "i-e" and "iae"
	if (len < 4) {
		if (!isdigit(encoded_value[1])) {
			throw std::runtime_error("Invalid encoded integer: " + encoded_value);
		}
	}
	if (!isdigit(encoded_value[1]) && encoded_value[1] != '-') {
		throw std::runtime_error("Invalid encoded integer: " + encoded_value);
	}
	
	// Find the 'e' index, if there is
	for (size_t i = 2; i < len; i++) {
		if (isdigit(encoded_value[i])) {
			continue;
		}

		if (encoded_value[i] == 'e') {
			index_end = i;
			break;
		}

		throw std::runtime_error("Invalid encoded integer: missing 'e' > " + encoded_value);
	}

	if (!index_end) {
		throw std::runtime_error("Invalid encoded integer: missing 'e' > " + encoded_value);
	}

	std::string str = encoded_value.substr(1, index_end-1);
	int64_t num = std::stoll(str);

	encoded_value = encoded_value.substr(index_end+1, len-index_end-1);

	return json(num);
}

json decode_list(std::string& encoded_value) {
	size_t len = encoded_value.length();
	if (len < 2) {
		throw std::runtime_error("Invalid encoded list: too short > " + encoded_value);
	}

	if (encoded_value[1] == 'e') {
		encoded_value = encoded_value.substr(2);
		return json::array();
	}

	// Remove 'l' from start
	encoded_value = encoded_value.substr(1);

	// Create a temp list with decoded values
	json temp_list = json::array();

	while (!encoded_value.empty() && encoded_value[0] != 'e') {
		temp_list.push_back(decode_bencoded_value(encoded_value));
	}

	if (encoded_value.empty()) {
		throw std::runtime_error("Invalid encoded list: missing 'e' > " + encoded_value);
	}

	// Remove remaining 'e' from start
	encoded_value = encoded_value.substr(1);

	return temp_list;
}

json decode_dictionary(std::string& encoded_value) {
	size_t len = encoded_value.length();
	if (len < 2) {
		throw std::runtime_error("Invalid encoded dictionary: too short > " + encoded_value);
	}

	if (encoded_value[1] == 'e') {
		encoded_value = "";
		return json::object();
	}

	// Remove 'd' from start
	encoded_value = encoded_value.substr(1);

	// Create a temp dictionary with encoded key/value pairs
	json temp_dict = json::object();
	
	while (!encoded_value.empty() && encoded_value[0] != 'e') {
		json key = decode_bencoded_value(encoded_value);
		if (!key.is_string()) {
			throw std::runtime_error("Invalid encoded dictionary: key must be string > " + encoded_value);
		}

		json value = decode_bencoded_value(encoded_value);
		temp_dict[key] = value;
	}

	if (encoded_value.empty()) {
		throw std::runtime_error("Invalid encoded dictionary: missing 'e' > " + encoded_value);
	}

	// Remove remaining 'e' from start
	encoded_value = encoded_value.substr(1);

	return temp_dict;
}

json decode_bencoded_value(std::string& encoded_value) {
	/* STRING */
	if (std::isdigit(encoded_value[0])) {
		// Example: "5:hello" -> "hello"
		return decode_string(encoded_value);
	}
	/* INTEGER */
	else if (encoded_value[0] == 'i') {
		// Examples: "i42e" -> 42 | "i-123e" -> -123
		return decode_integer(encoded_value);
	}
	/* LIST */
	else if (encoded_value[0] == 'l') {
		// Example: "l5:helloi52ee" -> ["hello",52] | li52e5:helloe -> [52,"hello"]
		return decode_list(encoded_value);
	}
	/* DICTIONARY */
	else if (encoded_value[0] == 'd') {
		// Example: "d3:foo3:bar5:helloi52ee" -> {"foo":"bar", "hello":52}
		return decode_dictionary(encoded_value);
	}
	else {
		throw std::runtime_error("Unhandled encoded value: " + encoded_value);
	}
}

int main(int argc, char* argv[]) {
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
		return 1;
	}

	std::string command = argv[1];

	if (command == "decode") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
			return 1;
		}

		std::string encoded_value = argv[2];
		json decoded_value = decode_bencoded_value(encoded_value);
		std::cout << decoded_value.dump() << std::endl;
	} else {
		std::cerr << "unknown command: " << command << std::endl;
		return 1;
	}

	return 0;
}
