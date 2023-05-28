# Encdec
A simple AES encoder/decoder for use with text files.
As a commandline tool, the following options are available:

* **-h, --help**: Disable options help.
* **-m, --mode**: Set the operation mode. Valid options are encode and decode.
* **-k, --key**: The encode/decode key, up to 16 characters long. Do not forget what you use as a key!
* **-a, -all**: Work on all the given txt (for encode) or bin (for decode) files in the local directory.
* **-i, --input**: Input filename (unless -a is set).
* **-o, --output**: Output filename (unless -a is set).

## Example usage
Open a commandline and to encode or decode:
* **encdec** -i source.txt -o encrypted.bin -m encode -k my_key
* **encdec** -i encrypted.bin -o decrypted.txt -m decode -k my_key

Or, if you want to encode *all* of the text files in the current directory:
* **encdec** -a -m encode -k my_key
The above will generate encoded .bin files for each .txt found. Be careful, as this will overwrite existing files! The same applies to reverse operation, decoding.

Warning: Be careful to always remember/take note of your key!

# Libraries
Uses the following libraries from Github:
* plusaes: AES encryption/decryption
* popl: Command-line options

# Bugs/Requests
Please use the [GitHub issue tracker](https://github.com/alkisbkn/Encdec/issues) to submit bugs or request features.

# License
Copyright Pantelis Lekakis, 2023

Distributed under the terms of the MIT license, Encdec is free and open source software.
