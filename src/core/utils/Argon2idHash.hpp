#pragma once

#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <argon2.h>
#include <vector>
#include <random>
#include <chrono>

#define HASHLEN 32
#define SALTLEN 16
#define T_COST 2 // 2-pass computation
#define M_COST 1 << 16 // 64 mebibytes memory usage
#define PARALLELISM 1 // number of threads and lanes

namespace Core::Utils
{
using random_bytes_engine = std::independent_bits_engine<
	std::default_random_engine, CHAR_BIT, unsigned short>;

static std::string argon2HashPassword(const std::string& password)
{
	// unsigned char salt[SALTLEN] = { 0x00 };
	random_bytes_engine rbe;
	rbe.seed(std::chrono::system_clock::now().time_since_epoch().count());
	std::vector<unsigned char> salt(SALTLEN);
	std::generate(begin(salt), end(salt), std::ref(rbe));

	const char* pwd = password.c_str();
	const unsigned int pwdlen = strlen(pwd);

	const unsigned int encodedLen = argon2_encodedlen(
		T_COST,
		M_COST,
		PARALLELISM,
		SALTLEN,
		HASHLEN,
		Argon2_id);
	std::vector<char> encodedHash(encodedLen);

	int result = argon2id_hash_encoded(
		T_COST,
		M_COST,
		PARALLELISM,
		pwd,
		pwdlen,
		salt.data(),
		SALTLEN,
		HASHLEN,
		encodedHash.data(),
		encodedLen);
	if (result != ARGON2_OK)
	{
		throw std::runtime_error(std::format("argon2id hashing didn't succeed, result {}", result));
	}

	// trim trailing zero byte
	while (encodedHash.back() == '\0')
	{
		encodedHash.pop_back();
	}

	return std::string(encodedHash.data());
}

static bool argon2VerifyEncodedHash(const std::string& hash, const std::string& password)
{
	int res = argon2id_verify(hash.data(), password.data(), password.length());
	if (res == ARGON2_OK)
		return true;
	else if (res == ARGON2_VERIFY_MISMATCH)
		return false;
	else
		throw std::runtime_error(std::format("argon2id verifying hash didn't succeed, result {}", res));
}
}