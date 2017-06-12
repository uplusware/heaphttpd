#include <stdio.h>
#include <string.h>
#include "util/general.h"
#include "util/security.h"

void usage()
{
	printf("Usage:heaptool --encode\n");
}

int main(int argc, char* argv[])
{
	if(argc == 2 && strcmp(argv[1], "--encode") == 0)
	{
		string strpwd = getpass("Input: ");

		string strOut;
		Security::Encrypt(strpwd.c_str(), strpwd.length(), strOut);
		printf("%s\n", strOut.c_str());
	}
	else
	{
		usage();
	}
}