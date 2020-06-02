#include <utility>

#include "certificate.h"

Certificate::Certificate(X509 *x509)
{
	_X509 = x509;

	// Increments the reference count
	X509_up_ref(_X509);
}

Certificate::~Certificate()
{
	OV_SAFE_FUNC(_X509, nullptr, X509_free,);
	OV_SAFE_FUNC(_pkey, nullptr, EVP_PKEY_free,);
}

bool Certificate::GenerateFromPem(std::string cert_filename, std::string private_key_filename)
{
	OV_ASSERT2(_X509 == nullptr);
	OV_ASSERT2(_pkey == nullptr);

	if((_X509 != nullptr) || (_pkey != nullptr))
	{
		//return ov::Error::CreateError("OpenSSL", 0, "Certificate is already created");
		return false;
	}

	// TODO(dimiden): If a cert file contains multiple certificates, it should be processed.
	// Read cert file
	BIO *cert_bio = nullptr;
	cert_bio = BIO_new(BIO_s_file());

	if(BIO_read_filename(cert_bio, cert_filename.c_str()) <= 0)
	{
		BIO_free(cert_bio);
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	_X509 = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
	BIO_free(cert_bio);

	if(_X509 == nullptr)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	// Read private key file
	BIO *pk_bio = nullptr;
	pk_bio = BIO_new(BIO_s_file());

	if(BIO_read_filename(pk_bio, private_key_filename.c_str()) <= 0)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	_pkey = PEM_read_bio_PrivateKey(pk_bio, nullptr, nullptr, nullptr);

	BIO_free(pk_bio);

	if(_pkey == nullptr)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	//return nullptr;
	return true;
}

bool Certificate::GenerateFromPem(std::string filename, bool aux)
{
	if(_X509 != nullptr)
	{
		//return ov::Error::CreateError("OpenSSL", 0, "Certificate is already created");
		return false;
	}

	BIO *cert_bio = nullptr;
	cert_bio = BIO_new(BIO_s_file());

	if(BIO_read_filename(cert_bio, filename.c_str()) <= 0)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	if(aux)
	{
		_X509 = PEM_read_bio_X509_AUX(cert_bio, nullptr, nullptr, nullptr);
	}
	else
	{
		_X509 = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
	}

	BIO_free(cert_bio);

	if(_X509 == nullptr)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	// TODO(dimiden): Extract these codes to another function like GenerateFromPrivateKey()
	//
	//_pkey = PEM_read_bio_PrivateKey(cert_bio, nullptr, nullptr, nullptr);
	//if(_pkey == nullptr)
	//{
	//	BIO_free(cert_bio);
	//	return ov::Error::CreateErrorFromOpenSsl();
	//}
	//
	//BIO_free(cert_bio);
	//
	//// Check Key
	//EC_KEY *ec_key = EVP_PKEY_get1_EC_KEY(_pkey);
	//if(!ec_key)
	//{
	//	return ov::Error::CreateErrorFromOpenSsl();
	//}
	//
	//if(!EC_KEY_check_key(ec_key))
	//{
	//	EC_KEY_free(ec_key);
	//	return ov::Error::CreateErrorFromOpenSsl();
	//}

	// return nullptr;
	return true;
}

bool Certificate::Generate()
{
	if(_X509 != nullptr)
	{
		//return ov::Error::CreateError("OpenSSL", 0, "Certificate is already created");
		return true;
	}

	EVP_PKEY *pkey = MakeKey();
	if(pkey == nullptr)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	X509 *x509 = MakeCertificate(pkey);
	if(x509 == nullptr)
	{
		//return ov::Error::CreateErrorFromOpenSsl();
		return false;
	}

	_pkey = pkey;
	_X509 = x509;

	// return nullptr;
	return true;
}

// Make ECDSA Key
EVP_PKEY *Certificate::MakeKey()
{
	EVP_PKEY *key;

	key = EVP_PKEY_new();
	if(key == nullptr)
	{
		return nullptr;
	}

	EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if(ec_key == nullptr)
	{
		EVP_PKEY_free(key);
		return nullptr;
	}

	EC_KEY_set_asn1_flag(ec_key, OPENSSL_EC_NAMED_CURVE);

	if(!EC_KEY_generate_key(ec_key) || !EVP_PKEY_assign_EC_KEY(key, ec_key))
	{
		EC_KEY_free(ec_key);
		EVP_PKEY_free(key);
		return nullptr;
	}

	return key;
}

// Make X509 Certificate
X509 *Certificate::MakeCertificate(EVP_PKEY *pkey)
{
	// Allocation
	X509 *x509 = X509_new();
	if(x509 == nullptr)
	{
		return nullptr;
	}

	// 버전 설정
	if(!X509_set_version(x509, 2L))
	{
		X509_free(x509);
		return nullptr;
	}

	// BIGNUM Allocation
	BIGNUM *serial_number = BN_new();
	if(serial_number == nullptr)
	{
		X509_free(x509);
		return nullptr;
	}

	// Allocation
	X509_NAME *name = X509_NAME_new();
	if(name == nullptr)
	{
		X509_free(x509);
		BN_free(serial_number);
		return nullptr;
	}

	// 공개키를 pkey로 설정
	if(!X509_set_pubkey(x509, pkey))
	{
		X509_free(x509);
		return nullptr;
	}

	ASN1_INTEGER *asn1_serial_number;

	// Random 값을 뽑아서 X509 Serial Number에 사용한다.
	BN_pseudo_rand(serial_number, 64, 0, 0);

	// 인증서 내부의 Serial Number를 반환 (내부값으로 해제되면 안됨)
	asn1_serial_number = X509_get_serialNumber(x509);

	// 상기 pseudo_rand로 생성한 serial_number를 ANS1_INTEGER로 변환하여 x509 내부 serial number 포인터에 바로 쓴다.
	BN_to_ASN1_INTEGER(serial_number, asn1_serial_number);

	// 인증서에 정보를 추가한다.
	if(!X509_NAME_add_entry_by_NID(name, NID_commonName, MBSTRING_UTF8, (unsigned char *)CERT_NAME, -1, -1, 0) ||
	   !X509_set_subject_name(x509, name) ||
	   !X509_set_issuer_name(x509, name))
	{
		X509_free(x509);
		BN_free(serial_number);
		X509_NAME_free(name);
		return nullptr;
	}

	// 인증서 유효기간 설정
	time_t epoch_off = 0;
	time_t now = time(nullptr);

	if(!X509_time_adj(X509_get_notBefore(x509), now + CertificateWindowInSeconds, &epoch_off) ||
	   !X509_time_adj(X509_get_notAfter(x509), now + DefaultCertificateLifetimeInSeconds, &epoch_off))
	{
		X509_free(x509);
		BN_free(serial_number);
		X509_NAME_free(name);
		return nullptr;
	}

	// Signing, 인증
	if(!X509_sign(x509, pkey, EVP_sha256()))
	{
		X509_free(x509);
		BN_free(serial_number);
		X509_NAME_free(name);
		return nullptr;
	}

	BN_free(serial_number);
	X509_NAME_free(name);
	return x509;
}

void Certificate::Print()
{
	BIO *temp_memory_bio = BIO_new(BIO_s_mem());

	if(!temp_memory_bio)
	{
		LOG_WRITE(("ERROR : CERT - Failed to allocate temporary memory bio"));
		return;
	}
	X509_print_ex(temp_memory_bio, _X509, XN_FLAG_SEP_CPLUS_SPC, 0);
	BIO_write(temp_memory_bio, "\0", 1);
	char *buffer;
	BIO_get_mem_data(temp_memory_bio, &buffer);
	//LOG_WRITE(("INFO : CERT - %s", buffer));
	BIO_free(temp_memory_bio);

	//LOG_WRITE(("INFO : CERT - Fingerprint sha-256 : %s", GetFingerprint("sha-256").c_str()));

	if(_pkey != nullptr)
	{
		BIO *bio_out = BIO_new_fp(stdout, BIO_NOCLOSE);
		//LOG_WRITE(("INFO : CERT - Public Key :"));
		EVP_PKEY_print_public(bio_out, _pkey, 0, NULL);
		//LOG_WRITE(("INOF : CERT - Private Key ::"));
		EVP_PKEY_print_private(bio_out, _pkey, 0, NULL);
		BIO_free(bio_out);
	}
}

bool Certificate::GetDigestEVP(const std::string &algorithm, const EVP_MD **mdp)
{
	const EVP_MD *md;

	if(algorithm == "md5")
	{
		md = EVP_md5();
	}
	else if(algorithm == "sha-1")
	{
		md = EVP_sha1();
	}
	else if(algorithm == "sha-224")
	{
		md = EVP_sha224();
	}
	else if(algorithm == "sha-256")
	{
		md = EVP_sha256();
	}
	else if(algorithm == "sha-384")
	{
		md = EVP_sha384();
	}
	else if(algorithm == "sha-512")
	{
		md = EVP_sha512();
	}
	else
	{
		return false;
	}

	*mdp = md;
	return true;
}

bool Certificate::ComputeDigest(const std::string algorithm)
{
	const EVP_MD *md;
	unsigned int n;

	if(!GetDigestEVP(algorithm, &md))
	{
		return false;
	}

	uint8_t digest[EVP_MAX_MD_SIZE];
	X509_digest(GetX509(), md, digest, &n);
	_digest.insert(_digest.end(), digest, digest + n);
	_digest_algorithm = algorithm;
	return true;
}

X509 *Certificate::GetX509()
{
	return _X509;
}

EVP_PKEY *Certificate::GetPkey()
{
	return _pkey;
}


std::string ToHexStringWithDelimiter(const void *data, size_t length, char delimiter)
{
	std::string dump;
	char text[MAX_PATH]; 

	const auto *buffer = static_cast<const uint8_t *>(data);

	for (int index = 0; index < length; index++)
	{
		sprintf(text, "%02X", *buffer);
		dump += text;

		if (index < length - 1)
		{
			dump.push_back(delimiter);
		}

		buffer++;
	}

	return dump;
}

//TODO(getroot): Algorithm을 enum값으로 변경
std::string Certificate::GetFingerprint(std::string algorithm)
{
	if(_digest.size() <= 0)
	{
		if(!ComputeDigest(std::move(algorithm)))
		{
			return "";
		}
	}

	std::string fingerprint = ToHexStringWithDelimiter(_digest.data(), _digest.size(), ':');

	std::transform(fingerprint.begin(), fingerprint.end(), fingerprint.begin(), ::toupper);

	return fingerprint;
}
