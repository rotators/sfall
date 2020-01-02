#pragma message("Compiling precompiled headers.\n")

#define HTTPD_SERVER

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#ifdef HTTPD_SERVER
	#define _WINSOCKAPI_
	#include <WinSock2.h>
	#include <WS2tcpip.h>
#endif
#include <Windows.h>
