/*
   MIT License

   Copyright (c) 2017-2018 Matthias C. M. Troffaes
   Copyright (c) 2018 Rotators

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

// This is modified version of IniPP library
// Original source https://github.com/mcmtroffaes/inipp/

#include <errno.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Rotators.Ini.h"

namespace rfall
{

static const char SECTION_START = '[';
static const char SECTION_END   = ']';
static const char KEY_ASSIGN    = '=';

static const std::vector<char> CommentChars = { '#', ';' };

inline void TrimLeft(std::string& str) {
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch){ return !std::isspace(ch); }));
}

inline void TrimRight(std::string& str) {
	str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch){ return !std::isspace(ch); }).base(), str.end());
}

inline void Cleanup(std::string& str) {
	str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	std::replace(str.begin(), str.end(), '\t', ' ');
}

//

Ini::Ini() : KeepComments(false), KeepSectionsRaw(false) {}

Ini::~Ini() {
	Unload();
}

//

bool Ini::LoadFile(const std::string& fname, bool unload /* = true */) {
	if (unload)
		Unload();

	std::ifstream fstream(fname);
	if (fstream.is_open()) {
		// strip BOM
		char bom[3] = { 0, 0, 0 };
		fstream.read(bom, sizeof(bom));
		if (bom[0] != (char)0xEF || bom[1] != (char)0xBB || bom[2] != (char)0xBF)
			fstream.seekg(0, fstream.beg);

		Parse(fstream);

		return true;
	}

	return false;
}

bool Ini::LoadString(const std::string& str, bool unload /* = true */) {
	if (unload)
		Unload();

	ParseString(str);

	return true;
}

void Ini::Unload() {
	Sections.clear();
}

//

void Ini::Parse(std::basic_istream<char>& stream) {
	std::string line;
	std::string section;

	while (!stream.eof()) {
		std::getline(stream, line);

		Cleanup(line);

		if (!KeepComments) {
			for (auto it = CommentChars.begin(), end = CommentChars.end(); it != end; ++it) {
				line = line.substr(0, line.find(*it));
			}
		}

		TrimLeft(line);
		TrimRight(line);

		const size_t length = line.length();

		if (!length || line.empty())
			continue;

		const size_t pos = line.find_first_of(KEY_ASSIGN);
		const char&  front = line.front();

		if (front == SECTION_START) {
			if (line.back() == SECTION_END)
				section = line.substr(1, length - 2);
		}
		else if (pos != 0 && pos != std::string::npos) {
			std::string key(line.substr(0, pos));
			std::string value(line.substr(pos + 1, length));

			TrimRight(key);
			TrimLeft(value);

			if (!IsSectionKey(section, key))
				SetStr(section, key, value);

			if (KeepSectionsRaw)
				AddSectionRaw(section, line);
		}
		else if (KeepSectionsRaw)
			AddSectionRaw(section, line);
	}
}

void Ini::ParseString(const std::string& str) {
	std::stringstream buf;
	buf << str;

	Parse(buf);
}

//

bool Ini::IsSection(const std::string& section) {
	return Sections.find(section) != Sections.end();
}

bool Ini::IsSectionKey(const std::string& section, const std::string& key) {
	if (!IsSection(section))
		return false;

	return Sections[section].find(key) != Sections[section].end();
}

bool Ini::IsSectionKeyEmpty(const std::string& section, const std::string& key) {
	if (!IsSectionKey(section, key))
		return true;

	return Sections[section][key].empty();
}

uint32_t Ini::GetSections(std::vector<std::string>& sections) {
	uint32_t count = 0;

	for (IniSections::iterator it = Sections.begin(); it != Sections.end(); ++it, count++) {
		sections.push_back(it->first);
	}

	return count;
}

uint32_t Ini::GetSectionKeys(const std::string& section, std::vector<std::string>& keys) {
	uint32_t count = 0;

	if (IsSection(section)) {
		for (IniSection::iterator it = Sections[section].begin(); it != Sections[section].end(); ++it, count++) {
			keys.push_back(it->first);
		}
	}

	return count;
}

//

bool Ini::MergeSections(const std::string& to, const std::string& from, bool overwrite /* = false */) {
	if (from == to || !IsSection(from))
		return false;

	std::vector<std::string> keys;
	for (uint32_t k = 0, kLen = GetSectionKeys(from, keys); k < kLen; k++) {
		if (overwrite || !IsSectionKey(to, keys[k])) {
			std::string key = keys[k];
			SetStr(to, key, Sections[from][key]);
		}
	}

	RemoveSection(from);
	return true;
}

bool Ini::RemoveSection(const std::string& section) {
	if (!IsSection(section))
		return false;

	Sections.erase(section);
	return true;
}

//

bool Ini::IsSectionRaw(const std::string& section) {
	return SectionsRaw.find(section) != SectionsRaw.end();
}

std::vector<std::string> Ini::GetSectionRaw(const std::string& section) {
	std::vector<std::string> result;

	if (IsSectionRaw(section))
		result = SectionsRaw[section];

	return result;
}

std::string Ini::GetSectionRawString(const std::string& section, const std::string& separator) {
	std::string result;

	bool first = true;
	std::vector<std::string> raw = GetSectionRaw(section);
	for (auto it = raw.begin(); it != raw.end(); ++it) {
		if (first)
			first = false;
		else
			result += separator;

		result += *it;
	}

	return result;
}

void Ini::AddSectionRaw(const std::string& section, const std::string& line) {
	if (line.empty())
		return;

	if (!IsSectionRaw(section)) {
		std::vector<std::string>& raw = SectionsRaw[section];
		raw.push_back(line);
	}
	else
		SectionsRaw[section].push_back(line);
}

//

bool Ini::GetBool(const std::string& section, const std::string& key, const bool& default_value) {
	bool result = default_value;

	if (!IsSectionKeyEmpty(section, key)) {
		std::string str = Sections[section][key];
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);

		result = (str == "1" || str == "yes" || str == "true" || str == "on" || str == "enable");
	}

	return result;
}

int32_t Ini::GetInt(const std::string& section, const std::string& key, const int32_t& default_value, const uint8_t& base /* = 10 */) {
	int32_t result = default_value;

	if (!IsSectionKeyEmpty(section, key)) {
		// https://stackoverflow.com/a/6154614
		const char* cstr = Sections[section][key].c_str();
		char*       end;
		long        l;
		errno = 0;
		// std::strtol ... DON'T! handling exceptions here would be silly
		l = ::strtol(cstr, &end, base);
		if (((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) ||
		    ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) ||
		     (*cstr == '\0' || *end != '\0'))
			result = default_value;
		else
			result = l;
	}

	return result;
}

int32_t Ini::GetOct(const std::string& section, const std::string& key, const int32_t& default_value) {
	return GetInt(section, key, default_value, 8);
}

int32_t Ini::GetHex(const std::string& section, const std::string& key, const int32_t& default_value) {
	return GetInt(section, key, default_value, 16);
}

std::string Ini::GetStr(const std::string& section, const std::string& key) {
	static const std::string empty;

	if (!IsSectionKeyEmpty(section, key))
		return std::string(Sections[section][key]);

	return empty;
}

std::string Ini::GetStr(const std::string& section, const std::string& key, const std::string& default_value) {
	if (!IsSectionKeyEmpty(section,key))
		return std::string(Sections[section][key]);

	return std::string(default_value);
}

std::vector<std::string> Ini::GetStrVec(const std::string& section, const std::string& key, char separator /* = ' ' */) {
	std::string              value = GetStr(section, key);
	std::vector<std::string> result;

	if (!value.empty()) {
		std::string        tmp;
		std::istringstream f(value);
		while (std::getline(f, tmp, separator)) {
			Cleanup(tmp);
			if (separator != ' ') {
				TrimLeft(tmp);
				TrimRight(tmp);
			}
			result.push_back(tmp);
		}
	}

	return result;
}

//

void Ini::SetStr(const std::string& section, const std::string& key, std::string value) {
	if (IsSectionKey(section, key))
		Sections[section][key] = value;
	else {
		IniSection& ini_section = Sections[section];
		ini_section.insert(std::make_pair(key, value));
	}
}

}
