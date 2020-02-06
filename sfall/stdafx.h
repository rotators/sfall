#pragma once
#pragma message("Compiling precompiled headers.\n")

#define WINVER       0x0501
#define _WIN32_WINNT 0x0501

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#define _WINSOCKAPI_
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
