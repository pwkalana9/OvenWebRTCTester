#pragma once

#include <openssl/ec.h>
#include <openssl/ssl.h>
#include "common_function.h"

#define OV_COUNTOF(arr)                             (sizeof(arr) / sizeof((arr)[0]))

// x가 value가 아니면, func를 호출한 뒤 value 값으로 만듦
#define OV_SAFE_FUNC(x, value, func, prefix) \
    do \
    { \
        if((x) != (value)) \
        { \
            func (prefix(x)); \
            (x) = (value); \
        } \
    } \
    while(false)

#define OV_SAFE_DELETE(x)                           OV_SAFE_FUNC(x, nullptr, delete,)
#define OV_SAFE_FREE(x)                             OV_SAFE_FUNC(x, nullptr, OV_GLOBAL_NAMESPACE_PREFIX free,) // NOLINT
#define OV_CHECK_FLAG(x, flag)                      (((x) & (flag)) == (flag))

#define OV_ASSERT(expression, format, ...)
#define OV_ASSERT2(expression)

#define    CERT_NAME        "OME"

static const int DefaultCertificateLifetimeInSeconds = 60 * 60 * 24 * 30 * 12;  // 1 years
static const int CertificateWindowInSeconds = -60 * 60 * 24;

enum class KeyType : int32_t
{
	Rsa,
	Ecdsa,
	Last,

	Default = Ecdsa
};

class Certificate
{
public:
	Certificate() = default;
	explicit Certificate(X509 *x509);
	~Certificate();

	bool Generate();
	bool GenerateFromPem(std::string cert_filename, std::string private_key_filename);
	// If aux flag is enabled, it will process a trusted X509 certificate using an X509 structure
	bool GenerateFromPem(std::string filename, bool aux);
	X509 *GetX509();
	EVP_PKEY *GetPkey();
	std::string GetFingerprint(std::string algorithm);

	// Print Cert for Test
	void Print();
private:
	// Make ECDSA Key
	EVP_PKEY *MakeKey();

	// Make Self-Signed Certificate
	X509 *MakeCertificate(EVP_PKEY *pkey);
	// Make Digest
	bool ComputeDigest(const std::string algorithm);

	bool GetDigestEVP(const std::string &algorithm, const EVP_MD **mdp);

	X509 *_X509 = nullptr;
	EVP_PKEY *_pkey = nullptr;

	std::vector<uint8_t> _digest;
	std::string _digest_algorithm;
};
