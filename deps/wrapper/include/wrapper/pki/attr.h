#ifndef CMS_PKI_ATTR_H_INCLUDED
#define  CMS_PKI_ATTR_H_INCLUDED

#include <openssl/asn1.h>
#include <openssl/x509.h>

#include "../common/common.h"

class CTWRAPPER_API Attribute;

#include "pki.h"
#include "oid.h"
#include "attr_vals.h"

SSLOBJECT_free(X509_ATTRIBUTE, X509_ATTRIBUTE_free);

class Attribute: public SSLObject<X509_ATTRIBUTE> {
public:
	SSLOBJECT_new(Attribute, X509_ATTRIBUTE){}
	SSLOBJECT_new_null(Attribute, X509_ATTRIBUTE, X509_ATTRIBUTE_new){}
	Attribute(Handle<OID> oid, int asnType);
	Attribute(const std::string& oid, int asnType);
	
	//-----Methods------
	Handle<Attribute> duplicate();
	Handle<std::string> write();

protected:
	//-----Properties-----
public:
	int getAsnType(); //get
	void setAsnType(int val); //set
	Handle<OID> getTypeId();
	void setTypeId(Handle<OID> &oid);
	void setTypeId(std::string oid);
	Handle<AttributeValueCollection> values();
	Handle<std::string> values(int index);

protected:
	int asnType_;
};

#endif //!CMS_PKI_ATTR_H_INCLUDED
