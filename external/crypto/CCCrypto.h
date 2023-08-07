#include <stdlib.h>
extern "C" {
#include "lua.h"
#include "tolua++.h"
}
#include "openssl/aes.h"
#include "base/base64.h"
#include "md5/md5.h"

NS_CC_EXTRA_BEGIN

class Crypto
{
public:
    static const int MD5_BUFFER_LENGTH = 16;

	static int getCypher(const char *content)
	{
		char * cypher = "_Fun Hospital_halo*365!D#";
		char buffer[512] = { 0 };
		sprintf(buffer, "%s%s", cypher, content);

		return MD5Lua(buffer, false);
	}
   
    
    /** @brief Calculate MD5, return MD5 string */
    static LUA_STRING MD5Lua(const char* input, bool isRawOutput);

    static LUA_STRING MD5FileLua(const char* path);
    
private:
    Crypto(void) {}
};

NS_CC_EXTRA_END

#endif // __CC_EXTENSION_CCCRYPTO_H_
