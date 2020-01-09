#include "HTML.h"
#include <cstdint>

namespace rfall {
	// Start indentation to be under <body>
	uint8_t HTMLIdentation = 2;

	// https://developer.mozilla.org/en-US/docs/Web/HTML/Element
	HTMLElement::~HTMLElement() {
		for (auto ch : children) {
			delete(ch);
		}
	}

	HTMLElement::HTMLElement(HTMLTag tag) {
		this->tag = tag;
	}

	HTMLElement::HTMLElement(HTMLTag tag, std::string innerHTML) {
		this->tag = tag;
		this->innerHTML = innerHTML;
	}

	HTMLElement* HTMLElement::add(HTMLElement* child) {
		this->children.push_back(child);
		return this;
	}

	std::string HTMLElement::getTagStr() {
		switch (tag) {
		case HTMLTag::a:     return "a";
		case HTMLTag::br:    return "br";
		case HTMLTag::div_:  return "div";
		case HTMLTag::body:  return "body";
		case HTMLTag::td:    return "td";
		case HTMLTag::tr:    return "tr";
		case HTMLTag::th:    return "th";
		case HTMLTag::table: return "table";
		case HTMLTag::tbody: return "tbody";
		case HTMLTag::thead: return "thead";
		case HTMLTag::p:     return "p";
		default: throw "invalid tag";
		}
	}

	bool HTMLElement::selfClosing() {
		switch (tag) {
		case HTMLTag::br: return true;
		default: return false;
		}
	}

	std::string HTMLElement::render() {
		auto tag = this->getTagStr();
		std::string buf("");
		buf += '\n';
		// make sure that stuff is properly indented in the generated source.
		for (auto i = 0; i < HTMLIdentation; i++) {
			buf += '\t';
		}

		buf += '<';
		buf += tag;
		if (attributes.size() > 0)
			buf += ' ';

		bool first = true;
		for (auto attr : attributes) {
			buf += attr.first;
			buf += '=';
			buf += '"';
			buf += attr.second;
			buf += '"';
			if (!first) {
				buf += ' ';
			}
			first = false;
		}
		if (selfClosing()) {
			buf += "/>";
			return buf;
		}
		else {
			buf += '>';
		}
		HTMLIdentation++;
		for (auto ch : children) {
			buf += ch->render();
		}
		HTMLIdentation--;
		buf += innerHTML;
		if (children.size() != 0) {
			buf += '\n';
			for (auto i = 0; i < HTMLIdentation; i++) {
				buf += '\t';
			}
		}
		buf += "</";
		buf += tag;
		buf += ">";
		return buf;
	}

	HTMLElement* HTMLElement::a(std::string innerHTML) { return new HTMLElement(HTMLTag::a, innerHTML); }
	HTMLElement* HTMLElement::div() { return new HTMLElement(HTMLTag::div_); }
	HTMLElement* HTMLElement::body() { return new HTMLElement(HTMLTag::body); }
	HTMLElement* HTMLElement::tbody() { return new HTMLElement(HTMLTag::tbody); }
	HTMLElement* HTMLElement::tbody(std::string innerHTML) { return new HTMLElement(HTMLTag::tbody, innerHTML); }
	HTMLElement* HTMLElement::thead() { return new HTMLElement(HTMLTag::thead); }
	HTMLElement* HTMLElement::td(std::string innerHTML) { return new HTMLElement(HTMLTag::td, innerHTML); }
	HTMLElement* HTMLElement::td() { return new HTMLElement(HTMLTag::td); }
	HTMLElement* HTMLElement::th(std::string innerHTML) { return new HTMLElement(HTMLTag::th, innerHTML); }
	HTMLElement* HTMLElement::tr() { return new HTMLElement(HTMLTag::tr); }
	HTMLElement* HTMLElement::p() { return new HTMLElement(HTMLTag::p); }
	HTMLElement* HTMLElement::p(std::string innerHTML) { return new HTMLElement(HTMLTag::p, innerHTML); }

	HTMLElement* HTMLTable::createHeader() {
		auto thead = new HTMLElement(HTMLTag::thead);
		auto tr = HTMLElement::tr();
		thead->children.push_back(tr);
		for (auto const& text : headers)
			tr->children.push_back(HTMLElement::th(text));
		return thead;
	}

	HTMLElement* HTMLTable::createBody() {
		this->tbody = HTMLElement::tbody();
		return this->tbody;
	}

	HTMLElement* HTMLTable::get() {
		auto table = new HTMLElement(HTMLTag::table);
		if (width != 0)
			table->attributes["style"] = "width: " + std::to_string(width) + "px";
		table->add(this->createHeader());
		table->add(this->tbody);
		return table;
	}

	HTMLElement* HTMLUtils::URL(std::string href, std::string text) {
		auto a = HTMLElement::a(text);
		a->attributes["href"] = href;
		return a;
	}

	
}