#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <functional>
#include "HTML.h"

namespace rfall
{
	class Router {
	public:
		void setContext(char* path, HTMLElement* root, HTMLElement* title);
		bool exactly(std::string url);
		void on(std::string _template, std::function<void(HTMLElement*, HTMLElement*, std::map<std::string, std::string>)> handler);
		bool matchTemplate(char decoded[1024], std::string _template);

		std::map<std::string, std::string> variables;
		HTMLElement* root;
		HTMLElement* title;
		char* path;

		bool handled;
	};
}
