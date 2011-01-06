/*	Better Oblivion Sorting Software
	
	Quick and Dirty Load Order Utility
	(Making C++ look like the scripting language it isn't.)

    Copyright (C) 2009-2010  Random/Random007/jpearce & the BOSS development team
    http://creativecommons.org/licenses/by-nc-nd/3.0/

	$Revision$, $Date$
*/

#include "Support/Logger.h"
#include "Userlist.h"
#include <boost/algorithm/string.hpp>
#define BOOST_FILESYSTEM_VERSION 3

namespace fs = boost::filesystem;


namespace boss {
	using namespace std;
	using boost::algorithm::to_lower_copy;
	using boost::algorithm::to_lower;
	using boost::algorithm::trim_copy;

	//Date comparison, used for sorting mods in modlist class.
	bool SortByDate(wstring mod1,wstring mod2) {
		time_t t1 = 0,t2 = 0;
		wstring utf16mod1 = utf8ToUTF16(mod1);
		wstring utf16mod2 = utf8ToUTF16(mod2);
		try {
			t1 = fs::last_write_time(data_path / utf16mod1);
			t2 = fs::last_write_time(data_path / utf16mod2);
		}catch (fs::filesystem_error e){
			LOG_WARN(L"%s; Report the mod in question with a download link to an official BOSS thread.", e.what());
		}
		double diff = difftime(t1,t2);

		if (diff > 0)
			return false;
		else
			return true;
	}

	//Checks if a given object is an esp, an esm or a ghosted mod.
	bool IsPlugin(wstring object) {
		to_lower(object);
		return (object.find(L".esp")!=string::npos || object.find(L".esm")!=string::npos);
	}

	//Prints messages generated by userlist parsing and rule processing to the output file stream given.
	void Rules::PrintMessages(wofstream& output) {
		output << messages;
	}

	bool PluginExists(fs::path plugin) {
		//return (fs::exists(plugin) || fs::exists(plugin.wstring()+L".ghost"));
		wstring utf16plugin = utf8ToUTF16(plugin.wstring());
		return (fs::exists(utf16plugin) || fs::exists(utf16plugin+L".ghost"));
	}

	//Add rules from userlist.txt into the rules object.
	//Then checks rule syntax and discards rules with incorrect structures.
	//Also checks if the mods referenced by rules are in your Data folder, and discards rule that reference missing mods.
	//Generates error messages for rules that are discarded.
	void Rules::AddRules() {
		wifstream userlist;
		wstring line,key,object;
		wchar_t cbuffer[MAXLENGTH];
		size_t pos;
		bool skip = false;
		unsigned short utf16BOM1 = 0xFE;
		unsigned short utf16BOM2 = 0xFF;
		bool invalidEnc = false;
		messages += L"<p>";
		userlist.open(userlist_path.c_str());
		while (!userlist.eof()) {
			userlist.getline(cbuffer,MAXLENGTH);
			line=cbuffer;
			if (line.length()>0) {
				if (line.substr(0,2)!=L"//") {
					if ((line[0] == utf16BOM1 && line[1] == utf16BOM2) || (line[0] == utf16BOM2 && line[1] == utf16BOM1)) {
						//Encoding is UTF-16 big endian. Userlist was saved in "Unicode big endian". Print error then stop parsing.
						messages += L"<span class='error'>Error: UTF-16 encoding detected. No rules will be applied. Re-save your userlist.txt in UTF-8 encoding. See Troubleshooting section of the ReadMe for more information.</span>";
						invalidEnc = true;
						break;
					} else if (!utf8::is_valid(line.begin(), line.end())) {
						//Encoding is not valid UTF-8.
						messages += L"<span class='error'>Error: The line \""+line+L"\" is not valid UTF-8. No rules will be applied. Re-save your userlist.txt in UTF-8 encoding. See Troubleshooting section of the ReadMe for more information.</span>";
						invalidEnc = true;
						break;
					}
					pos = line.find(L":");
					if (pos!=string::npos) {
						key = to_lower_copy(trim_copy(line.substr(0,pos)));
						//Remove UTF-8 BOM if present to prevent binary comparisons failing.
						if (utf8::starts_with_bom(key.begin(),key.end())) 
							key = key.substr(3);
						object = trim_copy(line.substr(pos+1));
						if (key==L"add" || key==L"override" || key==L"for") {
							if (skip) {
								keys.erase(keys.begin()+rules.back(), keys.end());
								objects.erase(objects.begin()+rules.back(), objects.end());
								rules.pop_back();
							}
							keys.push_back(key);
							objects.push_back(object);
							rules.push_back((int)keys.size()-1);
							skip = false;
							if (object.empty()) {
								if (!skip)  messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
								messages += L"<span class='error'>The line with keyword \""+key+L"\" has an undefined object.</span><br />";
								skip = true;
							} else {
								if (IsPlugin(object) && !PluginExists(data_path / object)) {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"\""+object+L"\" is not installed.<br />";
									skip = true;
								} else if (key==L"add" && !IsPlugin(object)) {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>It tries to add a group.</span><br />";
									skip = true;
								} else if (key==L"override") {
									if (Tidy(object)==L"esms") {
										if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
										messages += L"<span class='error'>It tries to sort the group \"ESMs\".</span><br />";
										skip = true;
									} else if (Tidy(object)==L"oblivion.esm" || Tidy(object)==L"fallout3.esm" || Tidy(object)==L"nehrim.esm" || Tidy(object)==L"falloutnv.esm") {
										if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
										messages += L"<span class='error'>It tries to sort the master .ESM file.</span><br />";
										skip = true;
									}
								}
							}
						} else if (!rules.empty()) {
							if ((key==L"before" || key==L"after")) {
								keys.push_back(key);
								objects.push_back(object);
								if (keys[rules.back()]==L"for") {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>It includes a sort line in a rule with a FOR rule keyword.</span><br />";
									skip = true;
								}
								if (object.empty()) {
									if (!skip)  messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>The line with keyword \""+key+L"\" has an undefined object.</span><br />";
									skip = true;
								} else {
									if (objects[rules.back()].length()>0 && ((IsPlugin(object) && !IsPlugin(objects[rules.back()])) || (!IsPlugin(object) && IsPlugin(objects[rules.back()])))) {
										if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
										messages += L"<span class='error'>It references a mod and a group.</span><br />";
										skip = true;
									}
									if (key==L"before") {
										if (Tidy(object)==L"esms") {
											if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
											messages += L"<span class='error'>It tries to sort a group before the group \"ESMs\".</span><br />";
											skip = true;
										} else if (Tidy(object)==L"oblivion.esm" || Tidy(object)==L"fallout3.esm" || Tidy(object)==L"nehrim.esm" || Tidy(object)==L"falloutnv.esm") {
											if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
											messages += L"<span class='error'>It tries to sort a mod before the master .ESM file.</span><br />";
											skip = true;
										}
									}
								}
							} else if ((key==L"top" || key==L"bottom")) {
								keys.push_back(key);
								objects.push_back(object);
								if (keys[rules.back()]==L"for") {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>It includes a sort line in a rule with a FOR rule keyword.</span><br />";
									skip = true;
								}
								if (object.empty()) {
									if (!skip)  messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>The line with keyword \""+key+L"\" has an undefined object.</span><br />";
									skip = true;
								} else {
									if (Tidy(object)==L"esms" && key==L"top") {
										if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
										messages += L"<span class='error'>It tries to insert a mod into the top of the group \"ESMs\", before the master .ESM file.</span><br />";
										skip = true;
									}
									if (objects[rules.back()].length()>0) {
										if (!IsPlugin(objects[rules.back()]) || IsPlugin(object)) {
											if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
											messages += L"<span class='error'>It tries to insert a group or insert a mod into another mod.</span><br />";
											skip = true;
										}
									}
								}
							} else if ((key==L"append" || key==L"replace")) {
								keys.push_back(key);
								objects.push_back(object);
								if (object.empty()) {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>The line with keyword \""+key+L"\" has an undefined object.</span><br />";
									skip = true;
								}
								if (!IsPlugin(objects[rules.back()])) {
									if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
									messages += L"<span class='error'>It tries to attach a message to a group.</span><br />";
									skip = true;
								}
							} else {
								//Line does not contain a recognised keyword. Skip it and the rule containing it. If it is a rule line, then the previous rule will also be skipped.
								if (!skip) messages += L"</p><p style='margin-left:40px; text-indent:-40px;'>The rule beginning \""+keys[rules.back()]+L": "+objects[rules.back()]+L"\" has been skipped as it has the following problem(s):<br />";
								messages += L"<span class='error'>The line \""+key+L": "+object+L"\" does not contain a recognised keyword. If this line is the start of a rule, that rule will also be skipped.</span><br />";
								skip = true;
							}
						} else {
							//Line is not a rule line, and appears before the first rule line, so does not belong to a rule. Skip it.
							if (key==L"before" || key==L"after" || key==L"top" || key==L"bottom" || key==L"append" || key==L"replace") 
								messages += L"<p><span class='error'>The line \""+key+L": "+object+L"\" appears before the first recognised rule line. Line skipped.</span><p>";
							else
								messages += L"<p><span class='error'>The line \""+key+L": "+object+L"\" does not contain a recognised keyword, and appears before the first recognised rule line. Line skipped.";
						}
					}
				}
			}
		}
		userlist.close();
		messages += L"</p>";
		if (skip) {
			keys.erase(keys.begin()+rules.back(), keys.end());
			objects.erase(objects.begin()+rules.back(), objects.end());
			rules.pop_back();
		}
		if (invalidEnc) {
			keys.clear();
			objects.clear();
			rules.clear();
		}
	}

	//Adds mods in directory to modlist in date order (AKA load order).
	void Mods::AddMods() {
		LOG_DEBUG(L"Reading user mods...");
		if (fs::is_directory(data_path)) {
			for (fs::directory_iterator itr(data_path); itr!=fs::directory_iterator(); ++itr) {
				const wstring utf16filename = itr->path().filename().wstring();
				const wstring utf8filename = utf16ToUTF8(utf16filename);
				const wstring ext = to_lower_copy(itr->path().extension().wstring());
				if (fs::is_regular_file(itr->status()) && (ext==L".esp" || ext==L".esm" || ext==L".ghost")) {
					LOG_TRACE(L"-- Found mod: '%s'", utf8filename.c_str());			
					mods.push_back(utf8filename);
				}
			}
		}
		modmessages.resize((int)mods.size());
		sort(mods.begin(),mods.end(),SortByDate);
		LOG_DEBUG(L"Reading user mods done: %" PRIuS L" total mods found.", mods.size());
	}

	//Save mod list to modlist.txt. Backs up old modlist.txt as modlist.old first.
	int Mods::SaveModList() {
		wofstream modlist;
		try {
			LOG_DEBUG(L"Saving backup of current modlist...");
			if (fs::exists(curr_modlist_path)) fs::rename(curr_modlist_path, prev_modlist_path);
		} catch(boost::filesystem::filesystem_error e) {
			//Couldn't rename the file, print an error message.
			LOG_ERROR(L"Backup of modlist failed with error: %s", e.what());
			return 1;
		}
		
		modlist.open(curr_modlist_path.c_str());
		//Provide error message if it can't be written.
		if (modlist.fail()) {
			LOG_ERROR(L"Backup cannot be saved.");
			return 2;
		}
		for (int i=0;i<(int)mods.size();i++) {
			modlist << mods[i] << endl;
		}
		modlist.close();
		LOG_INFO(L"Backup saved successfully.");
		return 0;
	}

	//Look for a mod in the modlist, even if the mod in question is ghosted.
	int Mods::GetModIndex(wstring mod) {
		for (int i=0;i<(int)mods.size();i++) {
			if (Tidy(mods[i])==Tidy(mod) || Tidy(mods[i])==Tidy(mod+L".ghost")) return i;
		}
		return -1;
	}
}