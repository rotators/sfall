#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Module.h"

#define DLL_EXPORT extern "C" __declspec(dllexport)

namespace fo
{

struct DbFile;
struct GameObject;

}

namespace rfall
{

class SubModuleManager {
private:
	std::vector<std::unique_ptr<sfall::Module>> _modules;

public:
	SubModuleManager()  = default;
	~SubModuleManager() = default;

private:
	// disallow copy constructor and copy assignment because we're dealing with unique_ptr's here
	SubModuleManager(const SubModuleManager&);
	void operator = (const SubModuleManager&) {}

public:
	void initAll();
	void exitAll();

	template<typename T>
	void add()
	{
		_modules.emplace_back(new T());
	}
};

struct Ini {
	static std::string String(const char* section, const char* setting, const char* defaultValue);
	static int Int(const char* section, const char* setting, int defaultValue);
	static std::vector<std::string> List(const char* section, const char* setting, const char* defaultValue, char delim = ',');
};

struct db {
	static void* readfile(char* filename, int len);
	static int filelen(fo::DbFile* file);
	static void* fastread(char* filename);
};

struct misc {
	enum class FLV : uint8_t {
		CRITTERS = 0,
		GROUNDITEMS,
		SCENERY,
		WALLS,
		TILES, // Disabled
		MISC,
		SPATIAL,
		ALL = 9
	};

	static void CriticalFail(const std::string& message);

	static void FillListVector(FLV type, std::vector<fo::GameObject*>& vec, int8_t elevation = -1); // -2=all -1=current 0-2=specific
};

}

namespace sfall
{

class Rotators : public Module {
private:
	rfall::SubModuleManager SubModules;

public:
	const char* name() { return "Rotators"; }
	void init();
	void exit() override;

	static void OnWmRefresh();
};

}
