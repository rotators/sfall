#include "HTML.h"
#include <cstdint>

namespace rfall {
	uint8_t HTMLIdentation = 0;

	// https://developer.mozilla.org/en-US/docs/Web/HTML/Element
	HTMLElement::~HTMLElement() {
		for (auto ch : children) {
			delete(ch);
		}
	}

	HTMLElement::HTMLElement(HTMLTag tag) {
		this->tag = tag;
	}

	HTMLElement::HTMLElement(HTMLTag tag, std::string text) {
		this->tag = tag;
		this->text = text;
	}

	// Short method to add child
	HTMLElement* HTMLElement::_(HTMLElement* child) {
		this->add(child);
		return this;
	}

	HTMLElement* HTMLElement::add(HTMLElement* child) {
		this->children.push_back(child);
		return this;
	}

	#define t(_a) case HTMLTag::_a:     return #_a;
	std::string HTMLElement::getTagStr() {
		switch (tag) {
			t(a)
			t(br)
			t(div)
			t(body)
			t(html)
			t(h1)
			t(h2)
			t(h3)
			t(h4)
			t(h5)
			t(h6)
			t(head)
			t(link)
			t(td)
			t(tr)
			t(th)
			t(table)
			t(tbody)
			t(thead)
			t(title)
			t(span)
			t(ol)
			t(ul)
			t(li)
			t(p)
		default: throw "invalid tag";
		}
	}

	bool HTMLElement::selfClosing() {
		switch (tag) {
		case HTMLTag::br: return true;
		case HTMLTag::link: return true;
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

		bool first = true;
		for (auto attr : attributes) {
			buf += ' ';
			buf += attr.first;
			buf += '=';
			buf += '"';
			buf += attr.second;
			buf += '"';
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
		buf += text;
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

	#define d(_e) HTMLElement* HTMLElement::##_e##()
	#define dt(_e) HTMLElement* HTMLElement::##_e##(std::string text)
	#define c(_e) { return new HTMLElement(HTMLTag::_e); }
	#define ct(_e) { return new HTMLElement(HTMLTag::_e, text); }
	#define dc(_e) d(_e) c(_e)    // Tag with no text element
	#define dtc(_e) dt(_e) ct(_e) // Tag with text element constructor
	#define b(_e) dc(_e) dtc(_e)  // Tag with both

	b(h1); b(h2); b(h3); b(h4); b(h5); b(h6);
	b(td)
	b(p)
	dc(head) dc(body)
	dc(div)
	dc(br)
	dc(tbody)
	dc(tr)
	dc(ul)
	dc(li)
	dtc(a)
	dtc(th)
	dtc(span)
	dtc(title)

	HTMLElement* HTMLTable::createHeader() {
		auto thead = new HTMLElement(HTMLTag::thead);
		auto tr = HTMLElement::tr();
		thead->children.push_back((tr));
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

	HTMLElement* HTMLUtils::CSS(std::string href) {
		auto l = new HTMLElement(HTMLTag::link);
		l->attributes["href"] = href;
		l->attributes["rel"] = "stylesheet";
		l->attributes["type"] = "text/css";
		return l;
	}
	HTMLElement* HTMLUtils::URL(std::string href, std::string text) {
		auto a = HTMLElement::a(text);
		a->attributes["href"] = href;
		return a;
	}

	
}