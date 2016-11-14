#ifndef UTIL_JWT_INCLUDED
#define UTIL_JWT_INCLUDED

#ifndef OPENSSL_NO_CTGOSTCP
	#include <openssl/license.h>
#endif

#include "../common/common.h"

class Jwt {
public:
	Jwt(){};
	~Jwt(){};

	bool checkLicense();
};

#endif //!UTIL_JWT_INCLUDED 
