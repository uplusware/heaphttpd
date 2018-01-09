#include "httputility.h"
#include "heapapi.h"

const char* http_utility::encode_URI(const char* src, string& dst)
{
	NIU_URLFORMAT_ENCODE((const unsigned char *) src, dst);
	return dst.c_str();
}

const char* http_utility::decode_URI(const char* src, string& dst)
{
	NIU_URLFORMAT_DECODE((const unsigned char *) src, dst);
	return dst.c_str();
}

const char* http_utility::escape_HTML(const char* src, string& dst)
{
	dst = src;
	Replace(dst, "&", "&amp;");
	
	Replace(dst, "\r", "");
	Replace(dst, "\n", "<BR>");
	Replace(dst, " ", "&nbsp;");
	Replace(dst, "\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
	
	Replace(dst, "<", "&lt;");
	Replace(dst, ">", "&gt;");
	Replace(dst, "'", "&apos;");
	Replace(dst, "\"", "&quot;");
	return dst.c_str();
}