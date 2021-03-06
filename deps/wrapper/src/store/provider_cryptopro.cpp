#include "../stdafx.h"

#include "wrapper/store/provider_cryptopro.h"

ProviderCryptopro::ProviderCryptopro(){
	LOGGER_FN();

	try{
		type = new std::string("CRYPTOPRO");
		providerItemCollection = new PkiItemCollection();

		init();
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Cannot be constructed ProviderCryptopro");
	}
}
void ProviderCryptopro::init(){
	LOGGER_FN();

	try{
		std::string listStore[] = {
			"MY",
			"AddressBook",
			"ROOT",
			"TRUST",
			"CA",
			"Request"
		};

		HCERTSTORE hCertStore;

		for (int i = 0, c = sizeof(listStore) / sizeof(*listStore); i < c; i++){
			std::wstring widestr = std::wstring(listStore[i].begin(), listStore[i].end());
			hCertStore = CertOpenStore(
				CERT_STORE_PROV_SYSTEM,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				NULL,
				CERT_SYSTEM_STORE_CURRENT_USER,
				widestr.c_str()
				);

			if (!hCertStore) {
				THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Error open store");
			}

			enumCertificates(hCertStore, &listStore[i]);
			enumCrls(hCertStore, &listStore[i]);

			CertCloseStore(hCertStore, 0);
			}
		}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error init CryptoPRO provider");
	}	
}

void ProviderCryptopro::enumCertificates(HCERTSTORE hCertStore, std::string *category){
	LOGGER_FN();

	try{
		X509 *cert = NULL;
		const unsigned char *p;

		if (!hCertStore){
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Certificate store not opened");
		}

		PCCERT_CONTEXT pCertContext = NULL;
		do
		{
			pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext);
			if (pCertContext){
				p = pCertContext->pbCertEncoded;
				LOGGER_OPENSSL(d2i_X509);
				if (!(cert = d2i_X509(NULL, &p, pCertContext->cbCertEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "'d2i_X509' Error decode len bytes");
				}

				Handle<Certificate> hcert = new Certificate(cert);
				Handle<PkiItem> item = objectToPKIItem(hcert);
				item->category = new std::string(*category);
				item->certificate = hcert;

				providerItemCollection->push(item);
			}			
		} while (pCertContext != NULL);

		if (pCertContext){
			CertFreeCertificateContext(pCertContext);
		}
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error enum certificates in store");
	}
}

void ProviderCryptopro::enumCrls(HCERTSTORE hCertStore, std::string *category){
	LOGGER_FN();

	try{
		X509_CRL *crl = NULL;
		const unsigned char *p;

		if (!hCertStore){
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Certificate store not opened");
		}

		PCCRL_CONTEXT pCrlContext = NULL;
		do
		{
			pCrlContext = CertEnumCRLsInStore(hCertStore, pCrlContext);
			if (pCrlContext){
				p = pCrlContext->pbCrlEncoded;
				LOGGER_OPENSSL(d2i_X509_CRL);
				if (!(crl = d2i_X509_CRL(NULL, &p, pCrlContext->cbCrlEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "'d2i_X509_CRL' Error decode len bytes");
				}			

				Handle<CRL> hcrl = new CRL(crl);
				Handle<PkiItem> item = objectToPKIItem(hcrl);
				item->category = new std::string(*category);
				item->crl = hcrl;

				providerItemCollection->push(item);
			}
		} while (pCrlContext != NULL);

		if (pCrlContext){
			CertFreeCRLContext(pCrlContext);
		}
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error enum CRLs in store");
	}
}

Handle<PkiItem> ProviderCryptopro::objectToPKIItem(Handle<Certificate> cert){
	LOGGER_FN();

	try{
		if (cert.isEmpty()){
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Certificate empty");
		}

		Handle<PkiItem> item = new PkiItem();

		item->format = new std::string("DER");
		item->type = new std::string("CERTIFICATE");
		item->provider = new std::string("CRYPTOPRO");

		char * hexHash;
		Handle<std::string> hhash = cert->getThumbprint();
		PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
		item->hash = new std::string(hexHash);

		item->certSubjectName = cert->getSubjectName();
		item->certSubjectFriendlyName = cert->getSubjectFriendlyName();
		item->certIssuerName = cert->getIssuerName();
		item->certIssuerFriendlyName = cert->getIssuerFriendlyName();
		item->certSerial = cert->getSerialNumber();
		item->certOrganizationName = cert->getOrganizationName();
		item->certSignatureAlgorithm = cert->getSignatureAlgorithm();

		item->certNotBefore = cert->getNotBefore();
		item->certNotAfter = cert->getNotAfter();
		item->certKey = hasPrivateKey(cert) ? new std::string("1") : new std::string("");

		return item;		
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error create PkiItem from certificate");
	}
}

Handle<PkiItem> ProviderCryptopro::objectToPKIItem(Handle<CRL> crl){
	LOGGER_FN();

	try{
		if (crl.isEmpty()){
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "CRL empty");
		}

		Handle<PkiItem> item = new PkiItem();

		item->format = new std::string("DER");
		item->type = new std::string("CRL");
		item->provider = new std::string("CRYPTOPRO");

		char * hexHash;
		Handle<std::string> hhash = crl->getThumbprint();
		PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
		item->hash = new std::string(hexHash);

		item->crlIssuerName = crl->issuerName();
		item->crlIssuerFriendlyName = crl->issuerFriendlyName();
		item->crlLastUpdate = crl->getThisUpdate();
		item->crlNextUpdate = crl->getNextUpdate();

		return item;
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error create PkiItem from crl");
	}
}

Handle<Certificate> ProviderCryptopro::getCert(Handle<std::string> hash, Handle<std::string> category){
	LOGGER_FN();

	X509 *hcert = NULL;

	try{
		HCERTSTORE hCertStore;
		PCCERT_CONTEXT pCertContext = NULL;
		
		const unsigned char *p;

		std::wstring wCategory = std::wstring(category->begin(), category->end());

		char cHash[20] = { 0 };
		hex2bin(hash->c_str(), cHash);

		CRYPT_HASH_BLOB cblobHash;
		cblobHash.pbData = (BYTE *)cHash;
		cblobHash.cbData = (DWORD)20;

		hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str()
			);

		if (!hCertStore) {
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Error open store");
		}

		pCertContext = CertFindCertificateInStore(
			hCertStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			0,
			CERT_FIND_HASH,
			&cblobHash,
			NULL);

		if (pCertContext) {
			p = pCertContext->pbCertEncoded;
			LOGGER_OPENSSL(d2i_X509);

			if (!(hcert = d2i_X509(NULL, &p, pCertContext->cbCertEncoded))) {
				THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "'d2i_X509' Error decode len bytes");
			}

			CertFreeCertificateContext(pCertContext);
				
			CertCloseStore(hCertStore, 0);

			return new Certificate(hcert);
		}
		else{
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Cannot find certificate in store");
		}
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error get certificate");
	}
}

Handle<CRL> ProviderCryptopro::getCRL(Handle<std::string> hash, Handle<std::string> category){
	LOGGER_FN();

	try{
		HCERTSTORE hCertStore;
		PCCRL_CONTEXT pCrlContext = NULL;

		const unsigned char *p;

		std::wstring wCategory = std::wstring(category->begin(), category->end());

		hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			NULL,
			CERT_SYSTEM_STORE_CURRENT_USER,
			wCategory.c_str()
			);

		if (!hCertStore) {
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Error open store");
		}

		do
		{
			pCrlContext = CertEnumCRLsInStore(hCertStore, pCrlContext);
			if (pCrlContext){
				X509_CRL *tempCrl = NULL;
				p = pCrlContext->pbCrlEncoded;
				LOGGER_OPENSSL(d2i_X509_CRL);
				if (!(tempCrl = d2i_X509_CRL(NULL, &p, pCrlContext->cbCrlEncoded))) {
					THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "'d2i_X509_CRL' Error decode len bytes");
				}

				Handle<CRL> hTempCrl = new CRL(tempCrl);

				char * hexHash;
				Handle<std::string> hhash = hTempCrl->getThumbprint();
				PkiStore::bin_to_strhex((unsigned char *)hhash->c_str(), hhash->length(), &hexHash);
				std::string sh(hexHash);

				if (strcmp(sh.c_str(), hash->c_str()) == 0){
					return hTempCrl;
				}
			}
		} while (pCrlContext != NULL);

		if (pCrlContext){
			CertFreeCRLContext(pCrlContext);
		}

		CertCloseStore(hCertStore, 0);

		THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Cannot find CRL in store");
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error get CRL");
	}
}

Handle<Key> ProviderCryptopro::getKey(Handle<Certificate> cert) {
	LOGGER_FN();

	EVP_PKEY_CTX *pctx = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;

	try{
#ifndef OPENSSL_NO_CTGOSTCP
#define MAX_SIGNATURE_LEN 128

		size_t len;
		unsigned char buf[MAX_SIGNATURE_LEN];

		ENGINE *e = ENGINE_by_id("ctgostcp");
		if (e == NULL) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "CTGSOTCP is not loaded");
		}			
			
	    LOGGER_OPENSSL(EVP_PKEY_CTX_new_id);
	    pctx = EVP_PKEY_CTX_new_id(NID_id_GostR3410_2001, e);

	    LOGGER_OPENSSL(EVP_PKEY_keygen_init);
	    if (EVP_PKEY_keygen_init(pctx) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "EVP_PKEY_keygen_init");
		}

	    LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
		if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_KEYSET, "all") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "EVP_PKEY_CTX_ctrl_str CTGOSTCP_PKEY_CTRL_STR_PARAM_KEYSET 'all'");
		}

	    LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
	    if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING, "true") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "EVP_PKEY_CTX_ctrl_str CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING 'true'");
		}

		LOGGER_OPENSSL(CTGOSTCP_EVP_PKEY_CTX_init_key_by_cert);
		if (CTGOSTCP_EVP_PKEY_CTX_init_key_by_cert(pctx, cert->internal()) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Can not init key context by certificate");
		}

		LOGGER_OPENSSL(EVP_PKEY_CTX_ctrl_str);
		if (EVP_PKEY_CTX_ctrl_str(pctx, CTGOSTCP_PKEY_CTRL_STR_PARAM_EXISTING, "true") <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Parameter 'existing' setting error");
		}

		LOGGER_OPENSSL(EVP_PKEY_keygen);
		if (EVP_PKEY_keygen(pctx, &pkey) <= 0){
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Can not init key by certificate");
		}

		int md_type = 0;
		const EVP_MD *md = NULL;

		LOGGER_OPENSSL(EVP_PKEY_get_default_digest_nid);
		if (EVP_PKEY_get_default_digest_nid(pkey, &md_type) <= 0) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "default digest for key type not found");
		}

		LOGGER_OPENSSL(EVP_get_digestbynid);
		md = EVP_get_digestbynid(md_type);

		LOGGER_OPENSSL(EVP_MD_CTX_create);
		if (!(mctx = EVP_MD_CTX_create())) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Error creating digest context");
		}

		len = sizeof(buf);
		LOGGER_OPENSSL(EVP_DigestSignInit);
		if (!EVP_DigestSignInit(mctx, NULL, md, e, pkey)
			|| (EVP_DigestSignUpdate(mctx, "123", 3) <= 0)
			|| !EVP_DigestSignFinal(mctx, buf, &len)) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Error testing private key (via signing data)");
		}

		return new Key(pkey);
#else
		THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Only with CTGSOTCP");
#endif
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error get key");
	}

	if (mctx) EVP_MD_CTX_destroy(mctx);
	if (pkey) EVP_PKEY_free(pkey);
	if (pctx) EVP_PKEY_CTX_free(pctx);
}

bool ProviderCryptopro::hasPrivateKey(Handle<Certificate> cert) {
	LOGGER_FN();

	try {
		PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
		PCCERT_CONTEXT pCertFound = HCRYPT_NULL;
		HCERTSTORE hCertStore = HCRYPT_NULL;
		DWORD dwSize = 0;
		bool res = false;

		pCertContext = createCertificateContext(cert);

		if (HCRYPT_NULL == (hCertStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			HCRYPT_NULL,
			CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
			L"My")))
		{
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "CertOpenStore(My) failed");
		}

		if (findExistingCertificate(pCertFound, hCertStore, pCertContext)) {
			if (CertGetCertificateContextProperty(pCertFound, CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwSize)) {
				res = true;
			}

			CertFreeCertificateContext(pCertFound);
			pCertFound = HCRYPT_NULL;
		}

		CertFreeCertificateContext(pCertContext);
		pCertContext = HCRYPT_NULL;

		CertCloseStore(hCertStore, 0);
		hCertStore = HCRYPT_NULL;

		return res;
	}
	catch (Handle<Exception> e) {
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error check key existing");
	}
}

PCCERT_CONTEXT ProviderCryptopro::createCertificateContext(Handle<Certificate> cert) {
	LOGGER_FN();

	try {
		PCCERT_CONTEXT pCertContext = HCRYPT_NULL;
		unsigned char *pData = NULL, *p = NULL;
		int iData;

		if (cert->isEmpty()) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "cert cannot be empty");
		}

		LOGGER_OPENSSL(i2d_X509);
		if ((iData = i2d_X509(cert->internal(), NULL)) <= 0) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Error i2d_X509");
		}

		LOGGER_OPENSSL(OPENSSL_malloc);
		if (NULL == (pData = (unsigned char*)OPENSSL_malloc(iData))) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Error malloc");
		}

		p = pData;
		LOGGER_OPENSSL(i2d_X509);
		if ((iData = i2d_X509(cert->internal(), &p)) <= 0) {
			THROW_OPENSSL_EXCEPTION(0, ProviderCryptopro, NULL, "Error i2d_X509");
		}

		LOGGER_TRACE("CertCreateCertificateContext");
		if (NULL == (pCertContext = CertCreateCertificateContext(
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pData, iData))) {
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "CertCreateCertificateContext() failed");
		}

		OPENSSL_free(pData);

		return pCertContext;
	}
	catch (Handle<Exception> e) {
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error create cert context from X509");
	}
}

bool ProviderCryptopro::findExistingCertificate(
	OUT PCCERT_CONTEXT &pOutCertContext,
	IN HCERTSTORE hCertStore,
	IN PCCERT_CONTEXT pCertContext,
	IN DWORD dwFindFlags,
	IN DWORD dwCertEncodingType
	) {

	LOGGER_FN();

	bool res = false;

	try {
		if (!hCertStore) {
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "certificate store cannot be empty");
		}

		if (!pCertContext) {
			THROW_EXCEPTION(0, ProviderCryptopro, NULL, "certificate context cannot be empty");
		}

		LOGGER_TRACE("CertFindCertificateInStore");
		pOutCertContext = CertFindCertificateInStore(
			hCertStore,
			dwCertEncodingType,
			dwFindFlags,
			CERT_FIND_EXISTING,
			pCertContext,
			NULL
			);

		if (pOutCertContext) {
			res = true;
		}

		return res;
	}
	catch (Handle<Exception> e) {
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error find certificate in store");
	}
}

int ProviderCryptopro::char2int(char input) {
	LOGGER_FN();

	try{
		if (input >= '0' && input <= '9'){
			return input - '0';
		}
			
		if (input >= 'A' && input <= 'F'){
			return input - 'A' + 10;
		}
			
		if (input >= 'a' && input <= 'f'){
			return input - 'a' + 10;
		}
		
		THROW_EXCEPTION(0, ProviderCryptopro, NULL, "Invalid input string");
	}
	catch (Handle<Exception> e){
		THROW_EXCEPTION(0, ProviderCryptopro, e, "Error char to int");
	}	
}

void ProviderCryptopro::hex2bin(const char* src, char* target) {
	LOGGER_FN();

	while (*src && src[1]){
		*(target++) = char2int(*src) * 16 + char2int(src[1]);
		src += 2;
	}
}
