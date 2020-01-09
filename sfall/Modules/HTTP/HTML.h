#pragma once
#include <map>
#include <string>
#include <vector>

namespace rfall
{
	enum HTMLTag {
		a,
		br,
		body,
		div_,
		html,
		td,
		tr,
		th,
		table,
		tbody,
		thead,
		p,
	};

	class HTMLElement {
	public:
		HTMLTag tag;
		std::map<std::string, std::string> attributes;
		std::string innerHTML;
		std::vector<HTMLElement*> children;
		HTMLElement(HTMLTag tag);
		HTMLElement(HTMLTag tag, std::string innerHTML);
		~HTMLElement();

		bool selfClosing();
		std::string HTMLElement::getTagStr();

		HTMLElement* add(HTMLElement* child);
		std::string render();

		static HTMLElement* a(std::string innerHTML);
		static HTMLElement* div();
		static HTMLElement* body();
		static HTMLElement* tbody();
		static HTMLElement* tbody(std::string innerHTML);
		static HTMLElement* thead();
		static HTMLElement* td(std::string innerHTML);
		static HTMLElement* td();
		static HTMLElement* th(std::string innerHTML);
		static HTMLElement* tr();
		static HTMLElement* p();
		static HTMLElement* p(std::string innerHTML);
	};

	class HTMLTable {
		public:
			int width = 0;
			std::vector<std::string> headers;
			HTMLElement* tbody;
			HTMLElement* createHeader();
			HTMLElement* createBody();
			HTMLElement* get();
	};

	class HTMLUtils {
		public:
			static HTMLElement* URL(std::string href, std::string text);
	};
};