// Copyright(c) 2023 Pantelis Lekakis
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
#include <filesystem>

#include "popl/include/popl.hpp"
#include "plusaes/plusaes.hpp"

enum class OperationMode : uint8_t
{
	Unknown,
	Encode,
	Decode
};

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

void ReportError(plusaes::Error const& error)
{
	if (error == plusaes::kErrorInvalidBufferSize)
		std::cerr << "AES error: Invalid buffer size";
	else if (error == plusaes::kErrorInvalidKeySize)
		std::cerr << "AES error: Invalid key size";
	else if (error == plusaes::kErrorInvalidDataSize)
		std::cerr << "AES error: Invalid data size";
	else if (error == plusaes::kErrorInvalidKey)
		std::cerr << "AES error: Invalid key";
	else if (error == plusaes::kErrorInvalidNonceSize)
		std::cerr << "AES error: Invalid N once size";
	else if (error == plusaes::kErrorInvalidIvSize)
		std::cerr << "AES error: Invalid Iv size";
	else if (error == plusaes::kErrorInvalidTagSize)
		std::cerr << "AES error: Invalid tag size";
	else if (error == plusaes::kErrorInvalidTag)
		std::cerr << "AES error: Invalid tag";
}

// Encode string with given key.
bool Encode(std::string const& str, std::vector<uint8_t>& encoded, std::string const& k)
{
	char keyval[17];
	memset(keyval, 0, sizeof(keyval));
	snprintf(keyval, k.size() + 1, k.c_str());
	auto const key = plusaes::key_from_string(&keyval);

	auto const encryptedSize = plusaes::get_padded_encrypted_size(str.size());
	std::vector<unsigned char> encrypted(encryptedSize);

	auto const error = plusaes::encrypt_cbc((unsigned char*)str.data(), str.size(), &key[0], key.size(), &iv, &encrypted[0], encrypted.size(), true);
	if (error != plusaes::kErrorOk)
	{
		std::cerr << "Encode errors:" << std::endl;
		ReportError(error);
		return false;
	}

	encoded = encrypted;
	return true;
}

// Decode string with given key.
bool Decode(std::vector<uint8_t> const& encoded, std::string& decoded, std::string const& k)
{
	char keyval[17];
	memset(keyval, 0, sizeof(keyval));
	snprintf(keyval, k.size()+1, k.c_str());
	
	auto const key = plusaes::key_from_string(&keyval);

	auto const encryptedSize = plusaes::get_padded_encrypted_size(encoded.size());

	unsigned long paddedSize = 0;
	std::vector<unsigned char> decrypted(encryptedSize);

	auto const error = plusaes::decrypt_cbc(&encoded[0], encoded.size(), &key[0], key.size(), &iv, &decrypted[0], decrypted.size(), &paddedSize);
	if (error != plusaes::kErrorOk)
	{
		std::cerr << "Decode errors:" << std::endl;
		ReportError(error);
		return false;
	}

	decoded = std::string(decrypted.begin(), decrypted.end());
	StringOps::Trim(decoded);

	return true;
}

bool Work(OperationMode mode, std::string const& input, std::string const& output, std::string const& key)
{
	std::cout << ((mode == OperationMode::Encode) ? "Encoding " : "Decoding ") << input << " to " << output << "..." << std::endl;

	// Open source file for reading.
	auto const readmode = mode == OperationMode::Decode ? (std::ios::in | std::ios::binary) : std::ios::in;
	std::ifstream inputFile(input, readmode);
	if (!inputFile.is_open())
	{
		std::cerr << "Cannot open input for reading!";
		return false;
	}

	// Open destination for writing.
	auto const writemode = mode == OperationMode::Encode ? (std::ios::out | std::ios::binary) : std::ios::out;
	std::ofstream outputFile(output, writemode);
	if (!outputFile.is_open())
	{
		std::cerr << "Cannot open output for writing!";
		return false;
	}

	// Perform encode/decode operation.
	if (mode == OperationMode::Encode)
	{
		std::stringstream source;
		source << inputFile.rdbuf();
		std::vector<uint8_t> encoded;
		Encode(source.str(), encoded, key);

		// Validate
		std::cout << "Validating..." << std::endl;

		std::string decoded;
		Decode(encoded, decoded, key);
		if (decoded != source.str())
		{
			std::cerr << "Decoded string doesn't match source after encoding! Skipping file write.";
			return false;
		}

		outputFile.write((char*)&encoded[0], encoded.size());
	}
	else if (mode == OperationMode::Decode)
	{
		std::vector<uint8_t> encoded((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
		std::string decoded;

		Decode(encoded, decoded, key);
		// Validate
		std::cout << "Validating..." << std::endl;

		std::vector<uint8_t> encoded2;
		Encode(decoded, encoded2, key);
		if (encoded != encoded2)
		{
			std::cerr << "Encoded string doesn't match source after decoding! Skipping file write.";
			return -1;
		}

		outputFile << decoded;
	}

	inputFile.close();
	outputFile.close();

	std::cout << "Success!";
}

int main(int argc, char* argv[])
{
	popl::OptionParser parser("Encdec options");
	auto help = parser.add<popl::Switch>("h", "help", "Show help about options");
	auto mode = parser.add<popl::Value<std::string>>("m", "mode", "Encoder/decoder mode (encode, decode)");
	auto key = parser.add<popl::Value<std::string>>("k", "key", "Encoder/decoder key");
	auto input = parser.add<popl::Value<std::string>>("i", "input", "Input filename");
	auto output = parser.add<popl::Value<std::string>>("o", "output", "Output filename");
	auto doall = parser.add<popl::Switch>("a", "all", "Iterate on all the expected input files (.txt for encoding, .bin for decoding)");
	parser.parse(argc, argv);

	if (help->is_set())
	{
		std::cout << parser << "\n";
		return 0;
	}

	if ((!input->is_set() || !output->is_set()) && !doall->is_set())
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

	std::string modeStr = mode->value();
	StringOps::ToLower(modeStr);

	OperationMode modeVal = OperationMode::Unknown;
	if (modeStr == "encode")
		modeVal = OperationMode::Encode;
	else if (modeStr == "decode")
		modeVal = OperationMode::Decode;
	else
	{
		std::cerr << "Invalid operation mode!";
		return -1;
	}

	bool success = true;
	// Iterate through all the expected filenames.
	if (doall->is_set())
	{
		namespace fs = std::filesystem;
		
		for (const auto& entry : fs::directory_iterator(fs::current_path()))
		{	
			auto filename = entry.path();

			if (!entry.is_regular_file() || !filename.has_extension() || !filename.has_filename())
				continue;
			
			std::string inputFilename, outputFilename;
			if (modeVal == OperationMode::Encode)
			{
				if (filename.extension() == ".txt")
				{
					inputFilename = filename.string();
					filename.replace_extension(".bin");
					outputFilename = filename.string();
				}
			}
			else if (modeVal == OperationMode::Decode)
			{
				if (filename.extension() == ".bin")
				{
					inputFilename = filename.string();
					filename.replace_extension(".txt");
					outputFilename = filename.string();
				}
			}

			if (!inputFilename.empty() && !outputFilename.empty())
			{
				success &= Work(modeVal, inputFilename, outputFilename, key->value());
			}
		}		
	}
	else
	{
		success &= Work(modeVal, input->value(), output->value(), key->value());
	}
	
	return success ? 0 : -1;
}