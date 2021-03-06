#include "../stdafx.h"

#include <node_buffer.h>

#include "wcert_request.h"
#include "wcert_request_info.h"
#include "wkey.h"

void WCertificationRequest::Init(v8::Handle<v8::Object> exports){
	METHOD_BEGIN();

	v8::Local<v8::String> className = Nan::New("CertificationRequest").ToLocalChecked();

	// Basic instance setup
	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);

	tpl->SetClassName(className);
	tpl->InstanceTemplate()->SetInternalFieldCount(1); // req'd by ObjectWrap

	Nan::SetPrototypeMethod(tpl, "load", Load);
	Nan::SetPrototypeMethod(tpl, "sign", Sign);
	Nan::SetPrototypeMethod(tpl, "getPEMString", GetPEMString);

	// Store the constructor in the target bindings.
	constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

	exports->Set(className, tpl->GetFunction());
}

NAN_METHOD(WCertificationRequest::New){
	METHOD_BEGIN();
	try{
		WCertificationRequest *obj = new WCertificationRequest();
		obj->data_ = new CertificationRequest();

		if (!info[0]->IsUndefined()){
			LOGGER_INFO("csrinfo");
			WCertificationRequestInfo * wCertRegInfo = WCertificationRequestInfo::Unwrap<WCertificationRequestInfo>(info[0]->ToObject());

			obj->data_ = new CertificationRequest(wCertRegInfo->data_);

		}
		
		obj->Wrap(info.This());

		info.GetReturnValue().Set(info.This());
		return;
	}
	TRY_END();	
}

/*
* filename: String
* format: DataFormat
*/
NAN_METHOD(WCertificationRequest::Load) {
	METHOD_BEGIN();

	try {
		LOGGER_ARG("filename");
		v8::String::Utf8Value v8Filename(info[0]->ToString());
		char *filename = *v8Filename;

		LOGGER_ARG("format");
		int format = info[1]->ToNumber()->Int32Value();

		UNWRAP_DATA(CertificationRequest);

		Handle<Bio> in = NULL;

		in = new Bio(BIO_TYPE_FILE, filename, "rb");

		_this->read(in, DataFormat::get(format));

		info.GetReturnValue().Set(info.This());
		return;
	}
	TRY_END();
}

NAN_METHOD(WCertificationRequest::Sign){
	METHOD_BEGIN();

	try{
		UNWRAP_DATA(CertificationRequest);

		LOGGER_ARG("key");
		WKey * wKey = WKey::Unwrap<WKey>(info[0]->ToObject());

		LOGGER_ARG("digest");
		v8::String::Utf8Value v8Digest(info[1]->ToString());
		char *digest = *v8Digest;
		std::string strDigest(digest);

		_this->sign(wKey->data_, strDigest.c_str());

		return;
	}
	TRY_END();
}

NAN_METHOD(WCertificationRequest::GetPEMString) {
	METHOD_BEGIN();

	try {
		UNWRAP_DATA(CertificationRequest);

		Handle<std::string> encCSR = _this->getPEMString();

		info.GetReturnValue().Set(stringToBuffer(encCSR));
		return;
	}
	TRY_END();
}