/*	BOSS
	
	A "one-click" program for users that quickly optimises and avoids 
	detrimental conflicts in their TES IV: Oblivion, Nehrim - At Fate's Edge, 
	TES V: Skyrim, Fallout 3 and Fallout: New Vegas mod load orders.

    Copyright (C) 2009-2012    BOSS Development Team.

	This file is part of BOSS.

    BOSS is free software: you can redistribute 
	it and/or modify it under the terms of the GNU General Public License 
	as published by the Free Software Foundation, either version 3 of 
	the License, or (at your option) any later version.

    BOSS is distributed in the hope that it will 
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BOSS.  If not, see 
	<http://www.gnu.org/licenses/>.

	$Revision: 2188 $, $Date: 2011-01-20 10:05:16 +0000 (Thu, 20 Jan 2011) $
*/

#include "Output/Output.h"
#include "Common/Error.h"
#include "Common/Globals.h"
#include "Support/Helpers.h"
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include "source/utf8.h"

namespace boss {
	using namespace std;
	using boost::algorithm::replace_all;
	using boost::locale::translate;

	////////////////////////////////
	// Outputter Class Functions
	////////////////////////////////

	Outputter::Outputter() 
		: outFormat(PLAINTEXT), 
		  escapeHTMLSpecialChars(false) {}

	Outputter::Outputter(const Outputter& o) {
		outStream << o.AsString();
		outFormat = o.GetFormat();
		escapeHTMLSpecialChars = o.GetHTMLSpecialEscape();
	}

	Outputter::Outputter(const uint32_t format) 
		: outFormat(format) {
		if (outFormat == HTML)
			escapeHTMLSpecialChars = true;
		else
			escapeHTMLSpecialChars = false;
	}

	Outputter::Outputter(const uint32_t format, const ParsingError e)
		: outFormat(format) {
		if (outFormat == HTML)
			escapeHTMLSpecialChars = true;
		else
			escapeHTMLSpecialChars = false;

		*this << e;
	}
	
	Outputter::Outputter(const uint32_t format, const Rule r)
		: outFormat(format) {
		if (outFormat == HTML)
			escapeHTMLSpecialChars = true;
		else
			escapeHTMLSpecialChars = false;

		*this << r;
	}

	Outputter::Outputter(const uint32_t format, const logFormatting l)
		: outFormat(format) {
		if (outFormat == HTML)
			escapeHTMLSpecialChars = true;
		else
			escapeHTMLSpecialChars = false;

		*this << l;
	}

	void Outputter::SetFormat(const uint32_t format) {
		outFormat = format;
		if (outFormat == HTML)
			escapeHTMLSpecialChars = true;
		else
			escapeHTMLSpecialChars = false;
	}

	void Outputter::SetHTMLSpecialEscape(const bool shouldEscape) {
		escapeHTMLSpecialChars = shouldEscape;
	}

	void Outputter::Clear() {
		outStream.str(std::string());
	}

	bool Outputter::Empty() const {
		return outStream.str().empty();
	}

	uint32_t Outputter::GetFormat() const {
		return outFormat;
	}

	bool Outputter::GetHTMLSpecialEscape() const {
		return escapeHTMLSpecialChars;
	}

	string Outputter::AsString() const {
		return outStream.str();
	}

	string Outputter::EscapeHTMLSpecial(string text) {
		if (escapeHTMLSpecialChars && outFormat == HTML) {
			replace_all(text, "&", "&amp;");
			replace_all(text, "\"", "&quot;");
			replace_all(text, "'", "&#039;");
			replace_all(text, "<", "&lt;");
			replace_all(text, ">", "&gt;");
			replace_all(text, "©", "&copy;");
			replace_all(text, "✗", "&#x2717;");
			replace_all(text, "✓", "&#x2713;");
			replace_all(text, "\n", "<br />");  //Not an HTML special char escape, but this needs to happen here to get the details of parser errors formatted correctly.
		}
		return text;
	}

	string Outputter::EscapeHTMLSpecial(char c) {
		if (escapeHTMLSpecialChars && outFormat == HTML) {
			switch(c) {
			case '&':
				return "&amp;";
			case '"':
				return "&quot;";
			case '\'':
				return "&#039;";
			case '<':
				return "&lt;";
			case '>':
				return "&gt;";
			case '©':
				return "&copy;";
			default:
				return string(1, c);
			}
		} else
			return string(1, c);
	}

	Outputter& Outputter::operator= (const Outputter& o) {
		outStream << o.AsString();
		outFormat = o.GetFormat();
		escapeHTMLSpecialChars = o.GetHTMLSpecialEscape();
		return *this;
	}
	
	Outputter& Outputter::operator<< (const string s) {
		outStream << EscapeHTMLSpecial(s);
		return *this;
	}

	Outputter& Outputter::operator<< (const char c) {
		outStream << EscapeHTMLSpecial(c);
		return *this;
	}

	Outputter& Outputter::operator<< (const char * s) {
		outStream << EscapeHTMLSpecial(s);
		return *this;
	}

	Outputter& Outputter::operator<< (const logFormatting l) {
		switch(l) {
		case SECTION_ID_SUMMARY_OPEN:
			if (outFormat == HTML)
				outStream << "<section id='summary'>";
			break;
		case SECTION_ID_USERLIST_OPEN:
			if (outFormat == HTML)
				outStream << "<section id='userRules'>";
			break;
		case SECTION_ID_SE_OPEN:
			if (outFormat == HTML)
				outStream << "<section id='sePlugins'>";
			break;
		case SECTION_ID_RECOGNISED_OPEN:
			if (outFormat == HTML)
				outStream << "<section id='recPlugins'>";
			break;
		case SECTION_ID_UNRECOGNISED_OPEN:
			if (outFormat == HTML)
				outStream << "<section id='unrecPlugins'>";
			break;
		case SECTION_CLOSE:
			if (outFormat == HTML)
				outStream << "</section>";
			break;
		case DIV_OPEN:
			if (outFormat == HTML)
				outStream << "<div>";
			else
				outStream << endl;
			break;
		case DIV_SUMMARY_BUTTON_OPEN:
			if (outFormat == HTML)
				outStream << "<div class='button' data-section='summary'>";
			break;
		case DIV_USERLIST_BUTTON_OPEN:
			if (outFormat == HTML)
				outStream << "<div class='button' data-section='userRules'>";
			break;
		case DIV_SE_BUTTON_OPEN:
			if (outFormat == HTML)
				outStream << "<div class='button' data-section='sePlugins'>";
			break;
		case DIV_RECOGNISED_BUTTON_OPEN:
			if (outFormat == HTML)
				outStream << "<div class='button' data-section='recPlugins'>";
			break;
		case DIV_UNRECOGNISED_BUTTON_OPEN:
			if (outFormat == HTML)
				outStream << "<div class='button' data-section='unrecPlugins'>";
			break;
		case DIV_CLOSE:
			if (outFormat == HTML)
				outStream << "</div>";
			break;
		case LINE_BREAK:
			if (outFormat == HTML)
				outStream << "<br />";
			else
				outStream << endl;
			break;
		case TABLE_HEAD:
			if (outFormat == HTML)
				outStream << "<thead>";
			break;
		case TABLE_HEADING:
			if (outFormat == HTML)
				outStream << "<th>";
			else
				outStream << '\t';
			break;
		case TABLE_BODY:
			if (outFormat == HTML)
				outStream << "<tbody>";
			break;
		case TABLE_ROW:
			if (outFormat == HTML)
				outStream << "<tr>";
			else
				outStream << endl;
			break;
		case TABLE_ROW_CLASS_SUCCESS:
			if (outFormat == HTML)
				outStream << "<tr class='success'>";
			else
				outStream << endl;
			break;
		case TABLE_ROW_CLASS_WARN:
			if (outFormat == HTML)
				outStream << "<tr class='warn'>";
			else
				outStream << endl;
			break;
		case TABLE_ROW_CLASS_ERROR:
			if (outFormat == HTML)
				outStream << "<tr class='error'>";
			else
				outStream << endl;
			break;
		case TABLE_DATA:
			if (outFormat == HTML)
				outStream << "<td>";
			else
				outStream  << '\t';
			break;
		case TABLE_OPEN:
			if (outFormat == HTML)
				outStream << "<table>";
			break;
		case TABLE_CLOSE:
			if (outFormat == HTML)
				outStream << "</table>";
			else
				outStream << endl << endl;
			break;
		case LIST_OPEN:
			if (outFormat == HTML)
				outStream << "<ul>";
			break;
		case LIST_CLOSE:
			if (outFormat == HTML)
				outStream << "</ul>";
			else
				outStream << endl;
			break;
		case HEADING_OPEN:
			if (outFormat == HTML)
				outStream << "<h2>";
			else
				outStream << endl << endl << "======================================" << endl;
			break;
		case HEADING_CLOSE:
			if (outFormat == HTML)
				outStream << "</h2>";
			else
				outStream << endl << "======================================" << endl << endl;
			break;
		case PARAGRAPH:
			if (outFormat == HTML)
				outStream << "<p>";
			else
				outStream << endl;
			break;
		case LIST_ITEM:
			if (outFormat == HTML)
				outStream << "<li>";
			else
				outStream << endl << endl;
			break;
		case LIST_ITEM_CLASS_SUCCESS:
			if (outFormat == HTML)
				outStream << "<li class='success'>";
			else
				outStream << endl << "*  ";
			break;
		case LIST_ITEM_CLASS_WARN:
			if (outFormat == HTML)
				outStream << "<li class='warn'>";
			else
				outStream << endl << "*  ";
			break;
		case LIST_ITEM_CLASS_ERROR:
			if (outFormat == HTML)
				outStream << "<li class='error'>";
			else
				outStream << endl << "*  ";
			break;
		case SPAN_ID_UNRECPLUGINSSUBMITNOTE_OPEN:
			if (outFormat == HTML)
				outStream << "<span id='unrecPluginsSubmitNote'>";
			break;
		case SPAN_CLASS_MOD_OPEN:
			if (outFormat == HTML)
				outStream << "<span class='mod'>";
			break;
		case SPAN_CLASS_VERSION_OPEN:
			if (outFormat == HTML)
				outStream << "<span class='version'>&nbsp;";
			else
				outStream << " ";
			break;
		case SPAN_CLASS_CRC_OPEN:
			if (outFormat == HTML)
				outStream << "<span class='crc'>&nbsp;";
			else
				outStream << " ";
			break;
		case SPAN_CLASS_ACTIVE_OPEN:
			if (outFormat == HTML)
				outStream << "<span class='active'>&nbsp;";
			else
				outStream << " ";
			break;
		case SPAN_CLASS_MESSAGE_OPEN:
			if (outFormat == HTML)
				outStream << "<span class='message'>";
			break;
		case SPAN_CLOSE:
			if (outFormat == HTML)
				outStream << "</span>";
			break;
		case ITALIC_OPEN:
			if (outFormat == HTML)
				outStream << "<i>";
			break;
		case ITALIC_CLOSE:
			if (outFormat == HTML)
				outStream << "</i>";
			break;
		case BLOCKQUOTE_OPEN:
			if (outFormat == HTML)
				outStream << "<blockquote>";
			else
				outStream << endl << endl;
			break;
		case BLOCKQUOTE_CLOSE:
			if (outFormat == HTML)
				outStream << "</blockquote>";
			else
				outStream << endl << endl;
			break;
		case VAR_OPEN:
			if (outFormat == HTML)
				outStream << "<var>";
			else
				outStream << "\"";
			break;
		case VAR_CLOSE:
			if (outFormat == HTML)
				outStream << "</var>";
			else
				outStream << "\"";
			break;
		default:
			break;
		}
		return *this;
	}
	
	Outputter& Outputter::operator<< (const int32_t i) {
		outStream << i;
		return *this;
	}
	
	Outputter& Outputter::operator<< (const uint32_t i) {
		outStream << i;
		return *this;
	}

	Outputter& Outputter::operator<< (const bool b) {
		if (b)
			outStream << "true";
		else
			outStream << "false";
		return *this;
	}

	Outputter& Outputter::operator<< (const fs::path p) {
		*this << p.string();
		return *this;
	}

	Outputter& Outputter::operator<< (const Message m) {
		string data = EscapeHTMLSpecial(m.Data());
		//Need to handle web addresses. Recognised are those in the following formats:
		//"http:someAddress", "http:someAddress label", "https:someAddress", "https:someAddress label", "file:somelocalAddress", "file:someLocalAddress label"
		
		size_t pos1,pos2,pos3;
		string link, label, dq;
		string addressTypes[] = {
			"http:",
			"https:",
			"file:"
		};

		if (outFormat == HTML)
			dq = "&quot;";
		else
			dq = "\"";

		//Do replacements for all addressTypes.
		for (uint32_t i=0; i < 2; i++) {
			pos1 = data.find(dq + addressTypes[i]);
			while (pos1 != string::npos) {
				pos1 += dq.length();
				pos3 = data.find(dq, pos1);  //End of quoted string.
				//Check if there is a label in the quoted string.
				pos2 = data.find(' ', pos1);
				if (pos2 < pos3) {  //Label present.
					link = data.substr(pos1, pos2-pos1);
					label = data.substr(pos2+1, pos3-pos2-1);
				} else {  //Label not present.
					link = data.substr(pos1, pos3-pos1);
					label = link;
				}
				if (outFormat == HTML)
					link = "<a href=\"" + link + "\">" + label + "</a>";
				else if (pos2 > pos3)
					link = '"' + link + '"';
				else
					link = label + " (\"" + link + "\")";
				data.replace(pos1 - dq.length(), pos3-pos1 + (2 * dq.length()), link);
				pos1 = data.find(dq + addressTypes[i], pos1);
			}
		}
		//Select message formatting.
		if (m.Key() == SAY) {
			if (outFormat == HTML)
				outStream << "<li class='note'>" << translate("Note") << ": " << data;
			else
				outStream << endl << "*  " << translate("Note") << ": " << data;
		} else if (m.Key() == TAG) {
			if (outFormat == HTML)
				outStream << "<li class='tag'><span class='tagPrefix'>" << translate("Bash Tag suggestion(s)") << ":</span> " << data;
			else
				outStream << endl << "*  " << translate("Bash Tag suggestion(s)") << ": " << data;
		} else if (m.Key() == REQ) {
			if (outFormat == HTML)
				outStream << "<li class='req'>" << translate("Requires") << ": " << data;
			else
				outStream << endl << "*  " << translate("Requires") << ": " << data;
		} else if (m.Key() == INC) {
			if (outFormat == HTML)
				outStream << "<li class='inc'>" << translate("Incompatible with") << ": " << data;
			else
				outStream << endl << "*  " << translate("Incompatible with") << ": " << data;
		} else if (m.Key() == WARN) {
			if (outFormat == HTML)
				outStream << "<li class='warn'>" << translate("Warning") << ": " << data;
			else
				outStream << endl << "*  " << translate("Warning") << ": " << data;
		} else if (m.Key() == ERR) {
			if (outFormat == HTML)
				outStream << "<li class='error'>" << translate("Error") << ": " << data;
			else
				outStream << endl << "*  " << translate("Error") << ": " << data;
		} else if (m.Key() == DIRTY) {
			if (outFormat == HTML)
				outStream << "<li class='dirty'>" << translate("Contains dirty edits") << ": " << data;
			else
				outStream << endl << "*  " << translate("Contains dirty edits") << ": " << data;
		} else {
			if (outFormat == HTML)
				outStream << "<li class='note'>" << translate("Note") << ": " << data;
			else
				outStream << endl << "*  " << translate("Note") << ": " << data;
		}
		return *this;
	}

	Outputter& Outputter::operator<< (const ParsingError e) {
		if (e.Empty())
			return *this;
		else if (!e.WholeMessage().empty())
			*this << LIST_ITEM_CLASS_ERROR << e.WholeMessage();
		else {
			bool htmlEscape = escapeHTMLSpecialChars;
			escapeHTMLSpecialChars = true;
			*this << LIST_ITEM_CLASS_ERROR << e.Header()
				<< BLOCKQUOTE_OPEN << e.Detail() << BLOCKQUOTE_CLOSE
				<< e.Footer();
			escapeHTMLSpecialChars = htmlEscape;
		}
		return *this;
	}

	Outputter& Outputter::operator<< (const Rule r) {
		bool hasEditedMessages = false;
		vector<RuleLine> lines = r.Lines();
		size_t linesSize = lines.size();
		string varOpen = Outputter(outFormat, VAR_OPEN).AsString();
		string varClose = Outputter(outFormat, VAR_CLOSE).AsString();

		//Need to temporarily turn off escaping of special characters so that <var> and </var> are printed correctly.
		bool wasEscaped = escapeHTMLSpecialChars;
		escapeHTMLSpecialChars = false;

		for (size_t j=0;j<linesSize;j++) {

			string rObject = varOpen + EscapeHTMLSpecial(r.Object()) + varClose;
			string lObject = varOpen + EscapeHTMLSpecial(lines[j].Object()) + varClose;

			if (lines[j].Key() == BEFORE)
				*this << (boost::format(translate("Sort %1% before %2%")) % rObject % lObject).str();
			else if (lines[j].Key() == AFTER)
				*this << (boost::format(translate("Sort %1% after %2%")) % rObject % lObject).str();
			else if (lines[j].Key() == TOP)
				*this << (boost::format(translate("Insert %1% at the top of %2%")) % rObject % lObject).str();
			else if (lines[j].Key() == BOTTOM)
				*this << (boost::format(translate("Insert %1% at the bottom of %2%")) % rObject % lObject).str();
			else if (lines[j].Key() == APPEND) {
				if (!hasEditedMessages) {
					if (r.Key() == FOR)
						*this << (boost::format(translate("Add the following messages to %1%:")) % rObject).str() << LINE_BREAK << LIST_OPEN;
					else
						*this << LINE_BREAK << translate("Add the following messages:") << LINE_BREAK << LIST_OPEN;
				}
				*this << lines[j].ObjectAsMessage();
				hasEditedMessages = true;
			} else if (lines[j].Key() == REPLACE) {
				if (!hasEditedMessages) {
					if (r.Key() == FOR)
						*this << (boost::format(translate("Replace the messages attached to %1% with:")) % rObject).str() << LINE_BREAK << LIST_OPEN;
					else
						*this << LINE_BREAK << translate("Replace the attached messages with:") << LINE_BREAK << LIST_OPEN;
				}
				*this << lines[j].ObjectAsMessage();
				hasEditedMessages = true;
			}
		}
		if (hasEditedMessages)
			*this << LIST_CLOSE;

		escapeHTMLSpecialChars = wasEscaped;

		return *this;
	}


	////////////////////////////////
	// Transcoder Class Functions
	////////////////////////////////

	Transcoder::Transcoder() {
		//Fill common character maps (1251/1252 -> UTF-8).
		commonMap.emplace('\x00', 0x0000);
		commonMap.emplace('\x01', 0x0001);
		commonMap.emplace('\x02', 0x0002);
		commonMap.emplace('\x03', 0x0003);
		commonMap.emplace('\x04', 0x0004);
		commonMap.emplace('\x05', 0x0005);
		commonMap.emplace('\x06', 0x0006);
		commonMap.emplace('\x07', 0x0007);
		commonMap.emplace('\x08', 0x0008);
		commonMap.emplace('\x09', 0x0009);
		commonMap.emplace('\x0A', 0x000A);
		commonMap.emplace('\x0B', 0x000B);
		commonMap.emplace('\x0C', 0x000C);
		commonMap.emplace('\x0D', 0x000D);
		commonMap.emplace('\x0E', 0x000E);
		commonMap.emplace('\x0F', 0x000F);
		commonMap.emplace('\x10', 0x0010);
		commonMap.emplace('\x11', 0x0011);
		commonMap.emplace('\x12', 0x0012);
		commonMap.emplace('\x13', 0x0013);
		commonMap.emplace('\x14', 0x0014);
		commonMap.emplace('\x15', 0x0015);
		commonMap.emplace('\x16', 0x0016);
		commonMap.emplace('\x17', 0x0017);
		commonMap.emplace('\x18', 0x0018);
		commonMap.emplace('\x19', 0x0019);
		commonMap.emplace('\x1A', 0x001A);
		commonMap.emplace('\x1B', 0x001B);
		commonMap.emplace('\x1C', 0x001C);
		commonMap.emplace('\x1D', 0x001D);
		commonMap.emplace('\x1E', 0x001E);
		commonMap.emplace('\x1F', 0x001F);
		commonMap.emplace('\x20', 0x0020);
		commonMap.emplace('\x21', 0x0021);
		commonMap.emplace('\x22', 0x0022);
		commonMap.emplace('\x23', 0x0023);
		commonMap.emplace('\x24', 0x0024);
		commonMap.emplace('\x25', 0x0025);
		commonMap.emplace('\x26', 0x0026);
		commonMap.emplace('\x27', 0x0027);
		commonMap.emplace('\x28', 0x0028);
		commonMap.emplace('\x29', 0x0029);
		commonMap.emplace('\x2A', 0x002A);
		commonMap.emplace('\x2B', 0x002B);
		commonMap.emplace('\x2C', 0x002C);
		commonMap.emplace('\x2D', 0x002D);
		commonMap.emplace('\x2E', 0x002E);
		commonMap.emplace('\x2F', 0x002F);
		commonMap.emplace('\x30', 0x0030);
		commonMap.emplace('\x31', 0x0031);
		commonMap.emplace('\x32', 0x0032);
		commonMap.emplace('\x33', 0x0033);
		commonMap.emplace('\x34', 0x0034);
		commonMap.emplace('\x35', 0x0035);
		commonMap.emplace('\x36', 0x0036);
		commonMap.emplace('\x37', 0x0037);
		commonMap.emplace('\x38', 0x0038);
		commonMap.emplace('\x39', 0x0039);
		commonMap.emplace('\x3A', 0x003A);
		commonMap.emplace('\x3B', 0x003B);
		commonMap.emplace('\x3C', 0x003C);
		commonMap.emplace('\x3D', 0x003D);
		commonMap.emplace('\x3E', 0x003E);
		commonMap.emplace('\x3F', 0x003F);
		commonMap.emplace('\x40', 0x0040);
		commonMap.emplace('\x41', 0x0041);
		commonMap.emplace('\x42', 0x0042);
		commonMap.emplace('\x43', 0x0043);
		commonMap.emplace('\x44', 0x0044);
		commonMap.emplace('\x45', 0x0045);
		commonMap.emplace('\x46', 0x0046);
		commonMap.emplace('\x47', 0x0047);
		commonMap.emplace('\x48', 0x0048);
		commonMap.emplace('\x49', 0x0049);
		commonMap.emplace('\x4A', 0x004A);
		commonMap.emplace('\x4B', 0x004B);
		commonMap.emplace('\x4C', 0x004C);
		commonMap.emplace('\x4D', 0x004D);
		commonMap.emplace('\x4E', 0x004E);
		commonMap.emplace('\x4F', 0x004F);
		commonMap.emplace('\x50', 0x0050);
		commonMap.emplace('\x51', 0x0051);
		commonMap.emplace('\x52', 0x0052);
		commonMap.emplace('\x53', 0x0053);
		commonMap.emplace('\x54', 0x0054);
		commonMap.emplace('\x55', 0x0055);
		commonMap.emplace('\x56', 0x0056);
		commonMap.emplace('\x57', 0x0057);
		commonMap.emplace('\x58', 0x0058);
		commonMap.emplace('\x59', 0x0059);
		commonMap.emplace('\x5A', 0x005A);
		commonMap.emplace('\x5B', 0x005B);
		commonMap.emplace('\x5C', 0x005C);
		commonMap.emplace('\x5D', 0x005D);
		commonMap.emplace('\x5E', 0x005E);
		commonMap.emplace('\x5F', 0x005F);
		commonMap.emplace('\x60', 0x0060);
		commonMap.emplace('\x61', 0x0061);
		commonMap.emplace('\x62', 0x0062);
		commonMap.emplace('\x63', 0x0063);
		commonMap.emplace('\x64', 0x0064);
		commonMap.emplace('\x65', 0x0065);
		commonMap.emplace('\x66', 0x0066);
		commonMap.emplace('\x67', 0x0067);
		commonMap.emplace('\x68', 0x0068);
		commonMap.emplace('\x69', 0x0069);
		commonMap.emplace('\x6A', 0x006A);
		commonMap.emplace('\x6B', 0x006B);
		commonMap.emplace('\x6C', 0x006C);
		commonMap.emplace('\x6D', 0x006D);
		commonMap.emplace('\x6E', 0x006E);
		commonMap.emplace('\x6F', 0x006F);
		commonMap.emplace('\x70', 0x0070);
		commonMap.emplace('\x71', 0x0071);
		commonMap.emplace('\x72', 0x0072);
		commonMap.emplace('\x73', 0x0073);
		commonMap.emplace('\x74', 0x0074);
		commonMap.emplace('\x75', 0x0075);
		commonMap.emplace('\x76', 0x0076);
		commonMap.emplace('\x77', 0x0077);
		commonMap.emplace('\x78', 0x0078);
		commonMap.emplace('\x79', 0x0079);
		commonMap.emplace('\x7A', 0x007A);
		commonMap.emplace('\x7B', 0x007B);
		commonMap.emplace('\x7C', 0x007C);
		commonMap.emplace('\x7D', 0x007D);
		commonMap.emplace('\x7E', 0x007E);
		commonMap.emplace('\x7F', 0x007F);
		commonMap.emplace('\x82', 0x201A);
		commonMap.emplace('\x84', 0x201E);
		commonMap.emplace('\x85', 0x2026);
		commonMap.emplace('\x86', 0x2026);
		commonMap.emplace('\x87', 0x2021);
		commonMap.emplace('\x89', 0x2030);
		commonMap.emplace('\x8B', 0x2039);
		commonMap.emplace('\x91', 0x2018);
		commonMap.emplace('\x92', 0x2019);
		commonMap.emplace('\x93', 0x201C);
		commonMap.emplace('\x94', 0x201D);
		commonMap.emplace('\x95', 0x2022);
		commonMap.emplace('\x96', 0x2013);
		commonMap.emplace('\x97', 0x2014);
		commonMap.emplace('\x99', 0x2122);
		commonMap.emplace('\x9B', 0x203A);
		commonMap.emplace('\xA0', 0x00A0);
		commonMap.emplace('\xA4', 0x00A4);
		commonMap.emplace('\xA6', 0x00A6);
		commonMap.emplace('\xA7', 0x00A7);
		commonMap.emplace('\xA9', 0x00A9);
		commonMap.emplace('\xAB', 0x00AB);
		commonMap.emplace('\xAC', 0x00AC);
		commonMap.emplace('\xAD', 0x00AD);
		commonMap.emplace('\xAE', 0x00AE);
		commonMap.emplace('\xB0', 0x00B0);
		commonMap.emplace('\xB1', 0x00B1);
		commonMap.emplace('\xB5', 0x00B5);
		commonMap.emplace('\xB6', 0x00B6);
		commonMap.emplace('\xB7', 0x00B7);
		commonMap.emplace('\xBB', 0x00BB);

		//Now fill 1252 -> UTF-8 map.
		map1252toUtf8 = commonMap;  //Fill with common mapped characters.
		map1252toUtf8.emplace('\x80', 0x20AC);
		map1252toUtf8.emplace('\x83', 0x0192);
		map1252toUtf8.emplace('\x88', 0x02C6);
		map1252toUtf8.emplace('\x8A', 0x0160);
		map1252toUtf8.emplace('\x8C', 0x0152);
		map1252toUtf8.emplace('\x8E', 0x017D);
		map1252toUtf8.emplace('\x98', 0x02DC);
		map1252toUtf8.emplace('\x9A', 0x0161);
		map1252toUtf8.emplace('\x9C', 0x0153);
		map1252toUtf8.emplace('\x9E', 0x017E);
		map1252toUtf8.emplace('\x9F', 0x0178);
		map1252toUtf8.emplace('\xA1', 0x00A1);
		map1252toUtf8.emplace('\xA2', 0x00A2);
		map1252toUtf8.emplace('\xA3', 0x00A3);
		map1252toUtf8.emplace('\xA5', 0x00A5);
		map1252toUtf8.emplace('\xA8', 0x00A8);
		map1252toUtf8.emplace('\xAA', 0x00AA);
		map1252toUtf8.emplace('\xAF', 0x00AF);
		map1252toUtf8.emplace('\xB2', 0x00B2);
		map1252toUtf8.emplace('\xB3', 0x00B3);
		map1252toUtf8.emplace('\xB4', 0x00B4);
		map1252toUtf8.emplace('\xB8', 0x00B8);
		map1252toUtf8.emplace('\xB9', 0x00B9);
		map1252toUtf8.emplace('\xBA', 0x00BA);
		map1252toUtf8.emplace('\xBC', 0x00BC);
		map1252toUtf8.emplace('\xBD', 0x00BD);
		map1252toUtf8.emplace('\xBE', 0x00BE);
		map1252toUtf8.emplace('\xBF', 0x00BF);
		map1252toUtf8.emplace('\xC0', 0x00C0);
		map1252toUtf8.emplace('\xC1', 0x00C1);
		map1252toUtf8.emplace('\xC2', 0x00C2);
		map1252toUtf8.emplace('\xC3', 0x00C3);
		map1252toUtf8.emplace('\xC4', 0x00C4);
		map1252toUtf8.emplace('\xC5', 0x00C5);
		map1252toUtf8.emplace('\xC6', 0x00C6);
		map1252toUtf8.emplace('\xC7', 0x00C7);
		map1252toUtf8.emplace('\xC8', 0x00C8);
		map1252toUtf8.emplace('\xC9', 0x00C9);
		map1252toUtf8.emplace('\xCA', 0x00CA);
		map1252toUtf8.emplace('\xCB', 0x00CB);
		map1252toUtf8.emplace('\xCC', 0x00CC);
		map1252toUtf8.emplace('\xCD', 0x00CD);
		map1252toUtf8.emplace('\xCE', 0x00CE);
		map1252toUtf8.emplace('\xCF', 0x00CF);
		map1252toUtf8.emplace('\xD0', 0x00D0);
		map1252toUtf8.emplace('\xD1', 0x00D1);
		map1252toUtf8.emplace('\xD2', 0x00D2);
		map1252toUtf8.emplace('\xD3', 0x00D3);
		map1252toUtf8.emplace('\xD4', 0x00D4);
		map1252toUtf8.emplace('\xD5', 0x00D5);
		map1252toUtf8.emplace('\xD6', 0x00D6);
		map1252toUtf8.emplace('\xD7', 0x00D7);
		map1252toUtf8.emplace('\xD8', 0x00D8);
		map1252toUtf8.emplace('\xD9', 0x00D9);
		map1252toUtf8.emplace('\xDA', 0x00DA);
		map1252toUtf8.emplace('\xDB', 0x00DB);
		map1252toUtf8.emplace('\xDC', 0x00DC);
		map1252toUtf8.emplace('\xDD', 0x00DD);
		map1252toUtf8.emplace('\xDE', 0x00DE);
		map1252toUtf8.emplace('\xDF', 0x00DF);
		map1252toUtf8.emplace('\xE0', 0x00E0);
		map1252toUtf8.emplace('\xE1', 0x00E1);
		map1252toUtf8.emplace('\xE2', 0x00E2);
		map1252toUtf8.emplace('\xE3', 0x00E3);
		map1252toUtf8.emplace('\xE4', 0x00E4);
		map1252toUtf8.emplace('\xE5', 0x00E5);
		map1252toUtf8.emplace('\xE6', 0x00E6);
		map1252toUtf8.emplace('\xE7', 0x00E7);
		map1252toUtf8.emplace('\xE8', 0x00E8);
		map1252toUtf8.emplace('\xE9', 0x00E9);
		map1252toUtf8.emplace('\xEA', 0x00EA);
		map1252toUtf8.emplace('\xEB', 0x00EB);
		map1252toUtf8.emplace('\xEC', 0x00EC);
		map1252toUtf8.emplace('\xED', 0x00ED);
		map1252toUtf8.emplace('\xEE', 0x00EE);
		map1252toUtf8.emplace('\xEF', 0x00EF);
		map1252toUtf8.emplace('\xF0', 0x00F0);
		map1252toUtf8.emplace('\xF1', 0x00F1);
		map1252toUtf8.emplace('\xF2', 0x00F2);
		map1252toUtf8.emplace('\xF3', 0x00F3);
		map1252toUtf8.emplace('\xF4', 0x00F4);
		map1252toUtf8.emplace('\xF5', 0x00F5);
		map1252toUtf8.emplace('\xF6', 0x00F6);
		map1252toUtf8.emplace('\xF7', 0x00F7);
		map1252toUtf8.emplace('\xF8', 0x00F8);
		map1252toUtf8.emplace('\xF9', 0x00F9);
		map1252toUtf8.emplace('\xFA', 0x00FA);
		map1252toUtf8.emplace('\xFB', 0x00FB);
		map1252toUtf8.emplace('\xFC', 0x00FC);
		map1252toUtf8.emplace('\xFD', 0x00FD);
		map1252toUtf8.emplace('\xFE', 0x00FE);
		map1252toUtf8.emplace('\xFF', 0x00FF);
		
		currentEncoding = 0;
	}

	void Transcoder::SetEncoding(const uint32_t inEncoding) {
		if (inEncoding != 1252)
			return;

		//Set the enc -> UTF-8 map.
		encToUtf8 = map1252toUtf8;
		currentEncoding = inEncoding;
		
		//Now create the UTF-8 -> enc map.
		for (boost::unordered_map<char, uint32_t>::iterator iter = encToUtf8.begin(); iter != encToUtf8.end(); ++iter) {
			utf8toEnc.emplace(iter->second, iter->first);  //Swap mapping. There *should* be unique values for each character either way.
		}
	}

	uint32_t Transcoder::GetEncoding() {
		return currentEncoding;
	}

	string Transcoder::Utf8ToEnc(const string inString) {
		stringstream outString;
		string::const_iterator strIter = inString.begin();
		//I need to use a UTF-8 string iterator. See UTF-CPP for usage.
		try {
			utf8::iterator<string::const_iterator> iter(strIter, strIter, inString.end());
			for (iter; iter.base() != inString.end(); ++iter) {
				boost::unordered_map<uint32_t, char>::iterator mapIter = utf8toEnc.find(*iter);
				if (mapIter != utf8toEnc.end())
					outString << mapIter->second;
				else
					throw boss_error(BOSS_ERROR_ENCODING_CONVERSION_FAIL, inString);
			}
		} catch (...) {
			throw boss_error(BOSS_ERROR_ENCODING_CONVERSION_FAIL, inString);
		}
		return outString.str();
	}

	string Transcoder::EncToUtf8(const string inString) {
		string outString;
		for (string::const_iterator iter = inString.begin(); iter != inString.end(); ++iter) {
			boost::unordered_map<char, uint32_t>::iterator mapIter = encToUtf8.find(*iter);
			if (mapIter != encToUtf8.end()) {
				try {
					utf8::append(mapIter->second, back_inserter(outString));
				} catch (...) {
					throw boss_error(BOSS_ERROR_ENCODING_CONVERSION_FAIL, inString);
				}
			} else
				throw boss_error(BOSS_ERROR_ENCODING_CONVERSION_FAIL, inString);
		}
		//Let's check that it's valid UTF-8.
		if (!utf8::is_valid(outString.begin(), outString.end()))
			throw boss_error(BOSS_ERROR_ENCODING_CONVERSION_FAIL, outString);
		return outString;
	}


	////////////////////////////////
	// BossLog Class Functions
	////////////////////////////////

	BossLog::BossLog() 
		: recognised(0), unrecognised(0), inactive(0), messages(0), warnings(0), errors(0), logFormat(HTML) {
		updaterOutput.SetFormat(HTML);
		criticalError.SetFormat(HTML);
		userRules.SetFormat(HTML);
		sePlugins.SetFormat(HTML);
		recognisedPlugins.SetFormat(HTML);
		unrecognisedPlugins.SetFormat(HTML);
	}

	BossLog::BossLog(const uint32_t format) 
		: recognised(0), unrecognised(0), inactive(0), messages(0), warnings(0), errors(0), logFormat(format) {
		updaterOutput.SetFormat(format);
		criticalError.SetFormat(format);
		userRules.SetFormat(format);
		sePlugins.SetFormat(format);
		recognisedPlugins.SetFormat(format);
		unrecognisedPlugins.SetFormat(format);
	}

	string BossLog::PrintHeaderTop() {
		stringstream out;
		if (logFormat == HTML)
			out << "<!DOCTYPE html>"
				<< "<meta charset='utf-8'>"
				<< "<title>BOSS Log</title>"
                << "<link rel='stylesheet' href='../resources/style.css' />"
				<< "<nav>"
				<< "<header>"
				<< "	<h1>BOSS</h1>"
				<< translate("	Version ") << IntToString(BOSS_VERSION_MAJOR) << "." << IntToString(BOSS_VERSION_MINOR) << "." << IntToString(BOSS_VERSION_PATCH)
				<< "</header>";
		else
			out << "\nBOSS\n"
				<< translate("Version ") << IntToString(BOSS_VERSION_MAJOR) << "." << IntToString(BOSS_VERSION_MINOR) << "." << IntToString(BOSS_VERSION_PATCH) << endl;
		return out.str();
	}

	string BossLog::PrintHeaderBottom() {
		stringstream out;
		if (logFormat == HTML)
			out << "<footer>"
				<< "	<div class='button' data-section='browserBox' id='supportButtonShow'>" << translate("Log Feature Support") << "</div>"
				<< "	<div class='button' id='filtersButtonToggle'>" << translate("Filters") << "<span id='arrow'></span></div>"
				<< "</footer>"
				<< "</nav>"
				<< "<noscript>"
				<< translate("The BOSS Log requires Javascript to be enabled in order to function.")
				<< "</noscript>";
		return out.str();
	}
	
	string BossLog::PrintFooter() {
		stringstream out;
		string colourTooltip = translate("Colours must be specified using lowercase hex codes.");

        if (logFormat == HTML)
            out << "<section id='browserBox'>"
            << "<p>" << translate("Support for the BOSS Log's more advanced features varies. Here's what your browser supports:")
            << "<h3>" << translate("Functionality") << "</h3>"
            << "<table>"
            << "	<tbody>"
            << "		<tr><td id='pluginSubmitSupport'><td>" << translate("In-Log Plugin Submission") << "<td>" << translate("Allows unrecognised plugins to be anonymously submitted to the BOSS team directly from the BOSS Log.")
            << "		<tr><td id='memorySupport'><td>" << translate("Settings Memory") << "<td>" << translate("Allows the BOSS Log to automatically restore the filter configuration last used whenever the BOSS Log is opened.")
            << "</table>"
            << "<h3>" << translate("Appearance") << "</h3>"
            << "<table>"
            << "	<tbody>"
            << "	<tr><td id='opacitySupport'><td>" << translate("Opacity")
            << "	<tr><td id='shadowsSupport'><td>" << translate("Shadows")
            << "	<tr><td id='transitionsSupport'><td>" << translate("Transitions")
            << "	<tr><td id='transformsSupport'><td>" << translate("Transforms")
            << "	<tr><td id='placeholderSupport'><td>" << translate("Input Placeholders")
            << "	<tr><td id='validationSupport'><td>" << translate("Form Validation")
            << "</table>"
            << "</section>"

            << "<aside>"
            << "<label><input type='checkbox' id='hideVersionNumbers' data-class='version'/>" << translate("Hide Version Numbers") << "</label>"
            << "<label><input type='checkbox' id='hideActiveLabel' data-class='active'/>" << translate("Hide 'Active' Label") << "</label>"
            << "<label><input type='checkbox' id='hideChecksums' data-class='crc'/>" << translate("Hide Checksums") << "</label>"
            << "<label><input type='checkbox' id='hideNotes'/>" << translate("Hide Notes") << "</label>"
            << "<label><input type='checkbox' id='hideBashTags'/>" << translate("Hide Bash Tag Suggestions") << "</label>"
            << "<label><input type='checkbox' id='hideRequirements'/>" << translate("Hide Requirements") << "</label>"
            << "<label><input type='checkbox' id='hideIncompatibilities'/>" << translate("Hide Incompatibilities") << "</label>"
            << "<label><input type='checkbox' id='hideDoNotCleanMessages'/>" << translate("Hide 'Do Not Clean' Messages") << "</label>"
            << "<label><input type='checkbox' id='hideAllPluginMessages'/>" << translate("Hide All Plugin Messages") << "</label>"
            << "<label><input type='checkbox' id='hideInactivePlugins'/>" << translate("Hide Inactive Plugins") << "</label>"
            << "<label><input type='checkbox' id='hideMessagelessPlugins'/>" << translate("Hide Messageless Plugins") << "</label>"
            << "<footer>"
            << "	" << (boost::format(translate("%1% of %2% recognised plugins hidden.")) % "<span id='hiddenPluginNo'>0</span>" % recognised).str()
            << "	" << (boost::format(translate("%1% of %2% messages hidden.")) % "<span id='hiddenMessageNo'>0</span>" % messages).str()
            << "</footer>"
            << "</aside>"

            << "<div id='overlay'>"
            << "<div id='submitBox'>"
            << "<h2>" << translate("Submit Plugin") << "</h2>"
            << "<p><span id='pluginLabel'>" << translate("Plugin") << ":</span><span id='plugin'></span>"
            << "<form>"
            << "<label>" << translate("Download Location") << ":<br /><input type='url' placeholder='" << translate("Label for text box. Do not use a single quote in translation, use '&#x27;' instead", "A link to the plugin&#x27;s download location.") << "' id='link'></label>"
            << "<label>" << translate("Additional Notes") << ":<br /><textarea id='notes' placeholder='" << translate("Any additional information, such as recommended Bash Tags, load order suggestions, ITM/UDR counts and dirty CRCs, can be supplied here. If no download link is available, this information is crucial.") << "'></textarea></label>"
            << "<div id='output'></div>"
            << "<p class='last'><button>" << translate("Submit") << "</button>"
            << "<button type='reset'>" << translate("Close") << "</button>"
            << "</form>"
            << "</div>"
            << "</div>"

            //Need to define some variables in code.
            << "<script>"
            << "var gameName = '" << gameName << "';"
            << "var txt1 = '" << translate("Checking for existing submission...") << "';"
            << "var txt2 = '" << translate("Error: Existing submission check failed!") << "';"
            << "var txt3 = '" << translate("Previous submission found, updating with supplied details...") << "';"
            << "var txt4 = '" << translate("Plugin submission updated!") << "';"
            << "var txt5 = '" << translate("Error: Plugin submission could not be updated.") << "';"
            << "var txt6 = '" << translate("No previous submission found, creating new submission...") << "';"
            << "var txt7 = '" << translate("Plugin submitted!") << "';"
            << "var txt8 = '" << translate("Error: Plugin could not be submitted.") << "';"
            << "var txt9 = '" << translate("Error: Data transfer failed.") << "';"
            << "var txt10 = '" << translate("Web storage quota for this document has been exceeded.Please empty your browser\\'s cache. Note that this will delete all locally stored data.") << "';"
            << "var txt11 = '" << translate("Please supply at least a link or some notes.") << "';"
            << "var txt12 = '" << translate("Exception occurred:") << "';"
            << "var txt13 = '" << translate("Do not clean.") << "';"
            << "</script>"
            << "<script src='../resources/script.js'></script>";
		return out.str();
	}

	string BossLog::PrintLog() {
		stringstream out;
		Outputter formattedOut(logFormat);

		// Print header
		//-------------------------

		out << PrintHeaderTop();

		if (logFormat == HTML) {
			formattedOut << DIV_SUMMARY_BUTTON_OPEN << translate("Summary") << DIV_CLOSE;

			if (!userRules.Empty())
				formattedOut << DIV_USERLIST_BUTTON_OPEN << translate("User Rules") << DIV_CLOSE;

			if (!sePlugins.Empty())
				formattedOut << DIV_SE_BUTTON_OPEN << (boost::format(translate("%1% Plugins")) % scriptExtender).str() << DIV_CLOSE;

			if (!recognisedPlugins.Empty()) {
				if (gl_revert < 1) 
					formattedOut << DIV_RECOGNISED_BUTTON_OPEN << translate("Recognised Plugins") << DIV_CLOSE;
				else
					formattedOut << DIV_RECOGNISED_BUTTON_OPEN << translate("Restored Load Order") << DIV_CLOSE;
			}

			if (!unrecognisedPlugins.Empty())
				formattedOut << DIV_UNRECOGNISED_BUTTON_OPEN << translate("Unrecognised Plugins") << DIV_CLOSE;

			out << formattedOut.AsString() << PrintHeaderBottom();
			formattedOut.Clear();  //Clear formattedOut for re-use.
		}


		// Print Summary
		//-------------------------

		formattedOut.SetHTMLSpecialEscape(false);

		if (logFormat == HTML)
			formattedOut << SECTION_ID_SUMMARY_OPEN;
		else
			formattedOut << SECTION_ID_SUMMARY_OPEN << HEADING_OPEN << translate("Summary") << HEADING_CLOSE;

		if (recognised != 0 || unrecognised != 0 || messages != 0) {
			formattedOut << TABLE_OPEN << TABLE_HEAD << TABLE_ROW << TABLE_HEADING << translate("Plugin Type") << TABLE_HEADING << translate("Count")
				<< TABLE_BODY << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("Recognised (or sorted by user rules)") << TABLE_DATA << recognised;
			if (unrecognised != 0)
				formattedOut << TABLE_ROW_CLASS_WARN << TABLE_DATA << translate("Unrecognised") << TABLE_DATA << unrecognised;
			else
				formattedOut << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("Unrecognised") << TABLE_DATA << unrecognised;
			formattedOut << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("Inactive") << TABLE_DATA << inactive
				<< TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("All") << TABLE_DATA << (recognised + unrecognised)
				<< TABLE_CLOSE

				<< TABLE_OPEN << TABLE_HEAD << TABLE_ROW << TABLE_HEADING << translate("Plugin Message Type") << TABLE_HEADING << translate("Count")
				<< TABLE_BODY;
			if (warnings != 0)
				formattedOut << TABLE_ROW_CLASS_WARN << TABLE_DATA << translate("Warning") << TABLE_DATA << warnings;
			else
				formattedOut << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("Warning") << TABLE_DATA << warnings;
			if (errors != 0)
				formattedOut << TABLE_ROW_CLASS_ERROR << TABLE_DATA << translate("Error") << TABLE_DATA << errors;
			else
				formattedOut << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("Error") << TABLE_DATA << errors;
			formattedOut << TABLE_ROW_CLASS_SUCCESS << TABLE_DATA << translate("All") << TABLE_DATA << messages
				<< TABLE_CLOSE;
		}

		formattedOut << LIST_OPEN;

		formattedOut << updaterOutput.AsString();  //This contains BOSS & masterlist update strings.

		if (recognisedHasChanged)
			formattedOut << LIST_ITEM_CLASS_SUCCESS << translate("No change in recognised plugin list since last run.");

		size_t size = parsingErrors.size();  //First print parser/syntax error messages.
		for (size_t i=0; i < size; i++)
			formattedOut << parsingErrors[i];

		formattedOut << criticalError.AsString();		//Print any critical errors.

		formattedOut.SetHTMLSpecialEscape(true);
		size = globalMessages.size();
		for (size_t i=0; i < size; i++)
			formattedOut << globalMessages[i];  //Print global messages.
		formattedOut.SetHTMLSpecialEscape(false);

		formattedOut << LIST_CLOSE << SECTION_CLOSE;
		out << formattedOut.AsString();
		formattedOut.Clear();  //Clear formattedOut for re-use.

		if (!criticalError.Empty()) {  //Exit early.
			out << PrintFooter();
			return out.str();
		}


		// Print User Rules
		//-------------------------

		if (!userRules.Empty()) {
			if (logFormat == HTML)
				formattedOut << SECTION_ID_USERLIST_OPEN;
			else
				formattedOut << SECTION_ID_USERLIST_OPEN << HEADING_OPEN << translate("User Rules") << HEADING_CLOSE;
			formattedOut << TABLE_OPEN << TABLE_HEAD << TABLE_ROW << TABLE_HEADING << translate("Rule") << TABLE_HEADING << translate("Applied") << TABLE_HEADING << translate("Details (if applicable)")
				<< TABLE_BODY << userRules.AsString() << TABLE_CLOSE << SECTION_CLOSE;
			out << formattedOut.AsString();
			formattedOut.Clear();
		}


		// Print Script Extender Plugins
		//--------------------------------------

		if (!sePlugins.Empty()) {
			if (logFormat == HTML)
				formattedOut << SECTION_ID_SE_OPEN;
			else
				formattedOut << SECTION_ID_SE_OPEN << HEADING_OPEN << scriptExtender << translate(" Plugins") << HEADING_CLOSE;
			formattedOut << LIST_OPEN
				<< sePlugins.AsString()
				<< LIST_CLOSE << SECTION_CLOSE;
			out << formattedOut.AsString();
			formattedOut.Clear();
		}


		// Print Recognised Plugins
		//-------------------------------

		if (!recognisedPlugins.Empty()) {
			if (logFormat == HTML)
				formattedOut << SECTION_ID_RECOGNISED_OPEN;
			else {
				if (gl_revert < 1) 
					formattedOut << SECTION_ID_RECOGNISED_OPEN << HEADING_OPEN << translate("Recognised Plugins") << HEADING_CLOSE;
				else if (gl_revert == 1)
					formattedOut << SECTION_ID_RECOGNISED_OPEN << HEADING_OPEN << translate("Restored Load Order (Using modlist.txt)") << HEADING_CLOSE;
				else if (gl_revert == 2) 
					formattedOut << SECTION_ID_RECOGNISED_OPEN << HEADING_OPEN << translate("Restored Load Order (Using modlist.old)") << HEADING_CLOSE;
			}
			formattedOut << PARAGRAPH 
				<< translate("These plugins are recognised by BOSS and have been sorted according to its masterlist. Please read any attached messages and act on any that require action.")
				<< LIST_OPEN
				<< recognisedPlugins.AsString()
				<< LIST_CLOSE << SECTION_CLOSE;
			out << formattedOut.AsString();
			formattedOut.Clear();
		}


		// Print Unrecognised Plugins
		//--------------------------------

		if (!unrecognisedPlugins.Empty()) {
			if (logFormat == HTML)
				formattedOut << SECTION_ID_UNRECOGNISED_OPEN;
			else
				formattedOut << SECTION_ID_UNRECOGNISED_OPEN << HEADING_OPEN << translate("Unrecognised Plugins") << HEADING_CLOSE;

			formattedOut << PARAGRAPH << translate("The following plugins were not found in the masterlist, and must be positioned manually, using your favourite mod manager or by using BOSS's user rules functionality.")
				<< SPAN_ID_UNRECPLUGINSSUBMITNOTE_OPEN << translate(" You can submit unrecognised plugins for addition to the masterlist directly from this log by clicking on a plugin and supplying a link and/or description of its contents in the panel that is displayed.") << SPAN_CLOSE << LIST_OPEN
				<< unrecognisedPlugins.AsString()
				<< LIST_CLOSE << SECTION_CLOSE;
			out << formattedOut.AsString();
			formattedOut.Clear();
		}


		// Print footer
		//-------------------------

		out << PrintFooter();
		
		return out.str();
	}

	void BossLog::SetFormat(const uint32_t format) {
		logFormat = format;
		updaterOutput.SetFormat(format);
		criticalError.SetFormat(format);
		userRules.SetFormat(format);
		sePlugins.SetFormat(format);
		recognisedPlugins.SetFormat(format);
		unrecognisedPlugins.SetFormat(format);
	}

	void BossLog::Save(const fs::path file, const bool overwrite) {
		if (fs::exists(file))
			recognisedHasChanged = HasRecognisedListChanged(file);

		ofstream outFile;
		if (overwrite)
			outFile.open(file.c_str());
		else
			outFile.open(file.c_str(), ios_base::out|ios_base::app);
		if (outFile.fail())
			throw boss_error(BOSS_ERROR_FILE_WRITE_FAIL, file.string());

		outFile << PrintLog();
		outFile.close();
	}

	void BossLog::Clear() {
		recognised = 0; 
		unrecognised = 0;
		inactive = 0;
		messages = 0;
		warnings = 0;
		errors = 0;

		scriptExtender.clear();
		gameName.clear();

		updaterOutput.Clear();
		criticalError.Clear();
		userRules.Clear();
		sePlugins.Clear();
		recognisedPlugins.Clear();
		unrecognisedPlugins.Clear();

		parsingErrors.clear();
		globalMessages.clear();
	}
	
	bool BossLog::HasRecognisedListChanged(const fs::path file) {
		size_t pos1, pos2;
		string result;
		fileToBuffer(file, result);
		if (logFormat == HTML) {
			pos1 = result.find("<section id='recPlugins'>");
			if (pos1 != string::npos)
				pos1 = result.find("<ul>", pos1);
			if (pos1 != string::npos)
				pos2 = result.find("</section>", pos1);
			if (pos2 != string::npos)
				result = result.substr(pos1+4, pos2-pos1-9);
		} else {
			pos1 = result.find(translate("Please read any attached messages and act on any that require action."));
			if (pos1 != string::npos)
				pos2 = result.find("======================================", pos1);
			if (pos2 != string::npos)
				result = result.substr(pos1+69, pos2-pos1-69-3);
		}
		return (result == recognisedPlugins.AsString());
	}
}