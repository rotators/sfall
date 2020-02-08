#pragma once
#pragma message("Compiling precompiled headers.\n")

#define WINVER       _WIN32_WINNT_WINXP
#define _WIN32_WINNT _WIN32_WINNT_WINXP

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>

//#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WINSOCKAPI_
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
