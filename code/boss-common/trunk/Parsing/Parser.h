/*	Better Oblivion Sorting Software
	
	Quick and Dirty Load Order Utility
	(Making C++ look like the scripting language it isn't.)

    Copyright (C) 2009-2010  Random/Random007/jpearce & the BOSS development team
    http://creativecommons.org/licenses/by-nc-nd/3.0/

	$Revision: 2188 $, $Date: 2011-01-20 10:05:16 +0000 (Thu, 20 Jan 2011) $
*/

//Contains functions for userlist and modlist/masterlist parsing.

#ifndef __BOSS_PARSER_H__
#define __BOSS_PARSER_H__

#include <string>
#include <vector>
#include "boost/filesystem.hpp"
#include "Common/Lists.h"
#include "Common/DllDef.h"

namespace boss {
	using namespace std;
	namespace fs = boost::filesystem;

	//Parses userlist into the given data structure.
	BOSS_COMMON bool parseUserlist(const fs::path file, vector<rule>& ruleList);

	//Parses the given masterlist into the given data structure. Also works for the modlist.
	BOSS_COMMON bool parseMasterlist(const fs::path file, vector<item>& modList);

	//Parses the ini, applying its settings.
	BOSS_COMMON bool parseIni(const fs::path file);

	//Reads an entire file into a string buffer.
	BOSS_COMMON void fileToBuffer(const fs::path file, string& buffer);

	//UTF-8 Validator
	BOSS_COMMON bool ValidateUTF8File(const fs::path file);
}
#endif