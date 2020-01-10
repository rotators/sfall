#include <map>
#include <string>
#include <functional>
#include <cstdint>

#include "..\..\Utils.h"
#include "HTML.h"
#include "Routing.h"

namespace rfall {

	enum RouteParserType {
		u8,
		u16,
		u32,
		i8,
		i16,
		i32,
		str
	};

	#define CHECK_TYPE(__type) if (spl[1].compare(#__type) == 0) tagtype = RouteParserType::__type
	// /loadmap/{id:u8}/{s:str}
	// Validate templates
	// {id:u8} = variable is named id and the type is uint8
	// Datatypes: u8, u16, u32, i8, i16, i32, str (char*)
	bool Router::matchTemplate(char decoded[1024], std::string _template) {
		bool parsingTag = false;
		std::string tagname("");
		RouteParserType tagtype;
		std::string tagbuf("");
		std::string matchbuf(""); // The decoded buffer that we compare with our tagbuf.
		char parseUntil = ' '; // What character is the next character after the tag ends?

		int pd = 0; // pos of decoded
		int pt = 0; // pos of template

		int maxLen = strlen(decoded);
		int parseState = 0;

		bool done = false;
		while (!done) {
			switch (parseState) {
			case 0: {
				if (_template[pt] == '{') {
					parseState = 1;
					continue;
				}
				if (decoded[pd++] != _template[pt++])
					return false;
			} break;
			case 1: { // Parse tag contents (everything in {})
				if (_template[pt] == '}') {
					parseState = 2;

					// Validate tag contents
					if (strstr(tagbuf.c_str(), ":") == NULL) {
						throw "no seperator : in tag";
					}
					auto spl = sfall::split(tagbuf, ':');
					tagname = spl[0].substr(1);
					matchbuf = "";

					CHECK_TYPE(u8); CHECK_TYPE(u16); CHECK_TYPE(u32); //CHECK_TYPE(u64);
					CHECK_TYPE(i8); CHECK_TYPE(i16); CHECK_TYPE(i32); //CHECK_TYPE(i64);
					CHECK_TYPE(str);

					if (pt + 1 == _template.size()) {
						parseUntil = '\0'; // Parse until the end
					}
					else {
						parseUntil = _template[pt + 1];
					}
					continue;
				}

				tagbuf += _template[pt++];
			} break;
			case 2: { // Compare with decoded string.
				if (decoded[pd] == parseUntil || pd == 1024) {
					try {
						if (tagtype != RouteParserType::str) {
							auto i = std::stoi(matchbuf);
							auto ui = std::stoul(matchbuf);
							switch (tagtype) {
							case u8: if (i < 0 || i > 255) return false;
								break;
							case u16: if (i < 0 || i > 65535) return false;
								break;
							case u32: if (i < 0) return false;
								break;
							case i8:  if (i < -128 || i > 127) return false;
								break;
							case i16: if (i < 0 || i > 65535) return false;
								break;
							case i32: if (ui > 2147483647) return false;
								break;
							}
						}
					}
					catch (...) { return false; } // Since value couldn't be parsed, it's not a matching route.

					// Add the captured variable
					variables[tagname] = matchbuf;

					// Are we done?
					if (pd < maxLen) {
						parseState = 0;
					}
					else {
						done = true;
					}
					continue;
				}

				matchbuf += decoded[pd++];
			} break;
			}
		}

		return true;
	}

	void Router::setContext(char* path, HTMLElement* root, HTMLElement* title) {
		this->path = path;
		this->root = root;
		this->title = title;
	}
	
	bool Router::exactly(std::string url) {
		return url.compare(this->path)==0;
	}

	void Router::on(std::string _template, std::function<void(HTMLElement*, HTMLElement *, std::map<std::string, std::string>)> handler) {
		if (this->matchTemplate(this->path, _template))
			handler(this->root, this->title, this->variables);
	};


}