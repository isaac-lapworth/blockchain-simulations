// an implementation of SHA-256, works on input strings that are a whole number of bytes 

#include <string>
#include <bitset>
#include <sstream>
#include <array>
#include <iomanip>

#include "SHA256.h"

// fractional parts of the cube roots of the first 64 primes
const unsigned int roundConstants[64] = {	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
											0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
											0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
											0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
											0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
											0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
											0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
											0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};


std::string pad(std::string data) {

	// take initial length of data (in bits), L
	long long int initialLength = data.size() * 8;

	// pad the data with a single 1 bit (actually 10000000)
	std::bitset<8> bitPad (0x80);
	data += static_cast<char>(bitPad.to_ulong());

	// pad with K zeroes until L + 1 + K + 64 is a multiple of 512, where K >= 0 (actually K >= 7)
	std::bitset<8> zeroPad(0x00);
	char zeroByte = static_cast<char>(zeroPad.to_ulong());
	long long int length = initialLength + 72;
	while (length % 512) {
		data += zeroByte;
		length += 8;
	}
	
	// add L as a 64 bit integer, so the final padded value has length of a multiple of 512
	std::bitset<64> initialLengthAsBits(initialLength);
	std::stringstream sstream(initialLengthAsBits.to_string());
	for (int i = 0; i < 8; ++i) {
		std::bitset<8> bits;
		sstream >> bits;
		data += static_cast<char>(bits.to_ulong());
	}
	return data;
}

unsigned int rightRotate(unsigned int x, int n) {
	return (x >> n) | (x << (32 - n));
}

void process(std::string data, unsigned int (&hashValues)[8]) {

	// convert data to unsigned ints that fill array entries 0-15
	std::array<unsigned int, 64> messageSchedule;
	for (int i = 0; i < 16; i++) {
		std::stringstream sstream;
		for (int j = 0; j < 4; j++) {
			sstream << std::bitset<8>(data[i * 4 + j]);
		}
		std::bitset<32> value(sstream.str());
		messageSchedule[i] = value.to_ulong();
	}
	
	// fill array entries 16-63
	for (int i = 16; i < 64; i++) {
		unsigned int a = rightRotate(messageSchedule[i - 15], 7) ^ rightRotate(messageSchedule[i - 15], 18) ^ (messageSchedule[i - 15] >> 3);
		unsigned int b = rightRotate(messageSchedule[i - 2], 17) ^ rightRotate(messageSchedule[i - 2], 19) ^ (messageSchedule[i - 2] >> 10);
		messageSchedule[i] = messageSchedule[i - 16] + a + messageSchedule[i - 7] + b;
	}

	// initialize working variables
	unsigned int a = hashValues[0];
	unsigned int b = hashValues[1];
	unsigned int c = hashValues[2];
	unsigned int d = hashValues[3];
	unsigned int e = hashValues[4];
	unsigned int f = hashValues[5];
	unsigned int g = hashValues[6];
	unsigned int h = hashValues[7];

	// compression loop
	for (int i = 0; i < 64; i++) {
		unsigned int u = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
		unsigned int v = (e & f) ^ ((~e) & g);
		unsigned int w = h + u + v + roundConstants[i] + messageSchedule[i];

		unsigned int x = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
		unsigned int y = (a & b) ^ (a & c) ^ (b & c);
		unsigned int z = x + y;

		h = g;
		g = f;
		f = e;
		e = d + w;
		d = c;
		c = b;
		b = a;
		a = w + z;
	}

	// add compressed chuck to current hash value
	hashValues[0] += a;
	hashValues[1] += b;
	hashValues[2] += c;
	hashValues[3] += d;
	hashValues[4] += e;
	hashValues[5] += f;
	hashValues[6] += g;
	hashValues[7] += h;
}

std::string sha256(std::string data) {
	
	// initially the hash's value is the fractional parts of the square roots of the first 8 primes
	unsigned int hashValues[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
	
	// pad the data to give it length 512n bits
	data = pad(data);

	// process the padded data in 512-bit chunks
	for (int i = 63; i < data.size(); i += 64) {
		process(data.substr(i - 63, 64), hashValues);
	}

	// return the final hash
	std::string hash = "";
	for (int i = 0; i < 8; ++i) {
		std::stringstream sstream;
		sstream << std::setfill('0') << std::setw(8) << std::hex << hashValues[i];
		hash += sstream.str();
	}
	return hash;
}
