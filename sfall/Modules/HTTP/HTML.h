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
		div,
		h1,
		h2,
		h3,
		h4,
		h5,
		h6,
		head,
		html,
		link,
		span,
		td,
		tr,
		th,
		table,
		tbody,
		thead,
		title,
		p,
		ul,
		ol,
		li
	};

	class HTMLElement {
	public:
		HTMLTag tag;
		std::map<std::string, std::string> attributes;
		std::string text;
		std::vector<HTMLElement*> children;
		HTMLElement(HTMLTag tag);
		HTMLElement(HTMLTag tag, std::string text);
		~HTMLElement();

		bool selfClosing();
		std::string HTMLElement::getTagStr();

		HTMLElement* add(HTMLElement* child);
		HTMLElement* _(HTMLElement* child);
		std::string render();

		static HTMLElement* a(std::string text);
		static HTMLElement* br();
		static HTMLElement* div();
		static HTMLElement* body();
		static HTMLElement* h1();
		static HTMLElement* h1(std::string text);
		static HTMLElement* h2();
		static HTMLElement* h2(std::string text);
		static HTMLElement* h3();
		static HTMLElement* h3(std::string text);
		static HTMLElement* h4();
		static HTMLElement* h4(std::string text);
		static HTMLElement* h5();
		static HTMLElement* h5(std::string text);
		static HTMLElement* h6();
		static HTMLElement* h6(std::string text);
		static HTMLElement* head();
		static HTMLElement* span();
		static HTMLElement* span(std::string text);
		static HTMLElement* tbody();
		static HTMLElement* thead();
		static HTMLElement* title(std::string text);
		static HTMLElement* td(std::string text);
		static HTMLElement* td();
		static HTMLElement* th(std::string text);
		static HTMLElement* tr();
		static HTMLElement* p();
		static HTMLElement* ul();
		static HTMLElement* li();
		static HTMLElement* p(std::string text);
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
			static HTMLElement* CSS(std::string href);
	};
};