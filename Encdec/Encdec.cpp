// Copyright(c) 2003 Pantelis Lekakis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <sstream>

#include "popl/include/popl.hpp"
#include "plusaes/plusaes.hpp"

constexpr unsigned char iv[16] =
{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

namespace StringOps
{
	void TrimL(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return ch != '\0';
			}));
	}

	void TrimR(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return ch != '\0';
			}).base(), s.end());
	}

	void Trim(std::string& s)
	{
		TrimL(s);
		TrimR(s);
	}

	// Convert input string to lowercase.
	void ToLower(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	}
}

// Encode string with given key.
void Encode(std::string& str, std::string const& k)
{
	char keyval[17];
	strcpy_s(keyval, k.c_str());
	auto const key = plusaes::key_from_string(&keyval);

	auto const encryptedSize = plusaes::get_padded_encrypted_size(str.size());
	std::vector<unsigned char> encrypted(encryptedSize);

	plusaes::encrypt_cbc((unsigned char*)str.data(), str.size(), &key[0], key.size(), &iv, &encrypted[0], encrypted.size(), true);

	str = std::string(encrypted.begin(), encrypted.end());
}

// Decode string with given key.
void Decode(std::string& str, std::string const& k)
{
	char keyval[17];
	strcpy_s(keyval, k.c_str());
	auto const key = plusaes::key_from_string(&keyval);

	auto const encryptedSize = plusaes::get_padded_encrypted_size(str.size());
	std::vector<unsigned char> encrypted(str.begin(), str.end());

	unsigned long paddedSize = 0;
	std::vector<unsigned char> decrypted(encryptedSize);

	plusaes::decrypt_cbc(&encrypted[0], encrypted.size(), &key[0], key.size(), &iv, &decrypted[0], decrypted.size(), &paddedSize);

	str = std::string(decrypted.begin(), decrypted.end());
	StringOps::Trim(str);
}

int main(int argc, char* argv[])
{
	popl::OptionParser parser("Encdec options");
	auto mode = parser.add<popl::Value<std::string>>("m", "mode", "Encoder/decoder mode (encode, decode)");
	auto key = parser.add<popl::Value<std::string>>("k", "key", "Encoder/decoder key");
	auto input = parser.add<popl::Value<std::string>>("i", "input", "Input filename");
	auto output = parser.add<popl::Value<std::string>>("o", "output", "Output filename");
	parser.parse(argc, argv);

	if (!input->is_set() || !output->is_set())
	{
		std::cerr << "Input and/or output filename is not set!";
		return -1;
	}
	if (!mode->is_set())
	{
		std::cerr << "Mode is not set!";
		return -1;
	}
	if (!key->is_set())
	{
		std::cerr << "Encode/decode key is not set!";
		return -1;
	}
	if (key->value().size() > 16)
	{
		std::cerr << "Key string too long, up to 16 characters are allowed for 128bit encoding";
		return -1;
	}

	// Open source file for reading.
	std::ifstream inputFile(input->value());
	if (!inputFile.is_open())
	{
		std::cerr << "Cannot open input for reading!";
		return -1;
	}	

	std::string modeStr = mode->value();
	StringOps::ToLower(modeStr);

	// Read source and encode/decode
	std::stringstream source;
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(inputFile, line))
	{	
		lines.push_back(line);
	}
	for (auto i=0; i<lines.size(); ++i)
	{
		source << lines[i];
		if (i != lines.size() - 1)
		{
			source << std::endl;
		}
	}
	inputFile.close();

	std::string sourceStr = source.str();
	// Perform encode/decode operation.
	if (modeStr == "encode")
	{
		std::cout << "Encoding " << input->value() << " to " << output->value() << "..." << std::endl;

		Encode(sourceStr, key->value());
		// Validate
		std::cout << "Validating..." << std::endl;

		std::string decoded = sourceStr;		
		Decode(decoded, key->value());
		if (decoded != source.str())
		{
			std::cerr << "Decoded string doesn't match source after encoding! Skipping file write.";
			return -1;
		}
	}
	else if (modeStr == "decode")
	{
		std::cout << "Decoding " << input->value() << " to " << output->value() << "..." << std::endl;

		Decode(sourceStr, key->value());
		// Validate
		std::cout << "Validating..." << std::endl;

		std::string encoded = sourceStr;		
		Encode(encoded, key->value());
		if (encoded != source.str())
		{
			std::cerr << "Encoded string doesn't match source after decoding! Skipping file write.";
			return -1;
		}
	}

	// WOpen destination for writing.
	std::ofstream outputFile(output->value());
	if (!outputFile.is_open())
	{
		std::cerr << "Cannot open output for writing!";
		return -1;
	}
	outputFile << sourceStr;
	outputFile.close();

	std::cout << "Success!";
	
	return 0;
}