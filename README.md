# Encdec
A simple AES encoder/decoder for use with text files.
As a commandline tool, the following options are available:

* **-h, --help**: Disable options help.
* **-m, --mode**: Set the operation mode. Valid options are encode and decode.
* **-k, --key**: The encode/decode key, up to 16 characters long. Do not forget what you use as a key!
* **-i, --input**: Input filename.
* **-o, --output**: Output filename.

## Example usage
Open a commandline and to encode or decode:
* **encdec** -i source.txt -o encrypted.txt -m encode -k my_key
* **encdec** -i encrypted.txt -o decrypted.txt -m decode -k my_key

# Libraries
Uses the following libraries from Github:
* plusaes: AES encryption/decryption
* popl: Command-line options

# Bugs/Requests
Please use the [GitHub issue tracker](https://github.com/alkisbkn/Encdec/issues) to submit bugs or request features.

# License
Copyright Pantelis Lekakis, 2023

Distributed under the terms of the MIT license, Encdec is free and open source software.
