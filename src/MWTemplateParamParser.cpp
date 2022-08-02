/**
 Copyright 2016 Myers Enterprises II

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <algorithm>
#include "MWTemplateParamParser.h"
#include "string_util.h"
#include <cctype>
#include <sstream>

using namespace std;

namespace phppreg {

map<string, PhpPreg> MWTemplateParamParser::regexs = {
		{"passed_param" , PhpPreg("!\\{\\{\\{(?P<content>[^{}]*?\\}\\}\\})!")}, // Highest priority
		{"htmlstub" , PhpPreg("!<\\s*(?P<content>[\\w]+(?:(?:\\s+\\w+(?:\\s*=\\s*(?:\"[^\"]*+\"|'[^']*+'|[^'\">\\s]+))?)+\\s*|\\s*)/>)!")},
		{"html" , PhpPreg("!<\\s*(?P<tag>[\\w]+)[^>]*>(?P<content>.*?<\\s*/\\s*(?P=tag)\\s*>)!s")},
		{"template" , PhpPreg("!\\{\\{\\s*(?P<content>(?P<name>[^{}\\|]+?)(?:\\|(?P<params>[^{}]+?))?\\}\\})!")},
		{"table" , PhpPreg("!\\{\\|(?P<content>[^{]*?\\|\\})!")},
		{"link" , PhpPreg("/\\[\\[(?P<content>(?:.(?!\\[\\[))+?\\]\\])/s")}
};

vector<string> MWTemplateParamParser::regexs_ordered = {
		"passed_param",
		"htmlstub",
		"html",
		"template",
		"table",
		"link"
};

const int MWTemplateParamParser::MAX_ITERATIONS = 1000;
PhpPreg MWTemplateParamParser::COMMENT_REGEX("/<!--.*?-->/us");
PhpPreg MWTemplateParamParser::MARKER_REGEX("!\\x02\\d+\\x03!");
PhpPreg MWTemplateParamParser::NOWIKI_REGEX("!<\\s*nowiki\\s*>.*?<\\s*/nowiki\\s*>!usi");
PhpPreg MWTemplateParamParser::BR_REGEX("!<\\s*br\\s*/?\\s*>!usi");

/**
 * Get template names and parameters in a string.
 * Numbered params are relative to 1
 *
 * @param results
 * @param page_data
 */
void MWTemplateParamParser::getTemplates(vector<MWTemplate> *results, const string& origdata)
{
		int itercnt = 0;
		bool match_found = true;
		map<string, string> markers;
		vector<string> templates;
		vector<shared_ptr<MatchVector>> marker_matches;
		MatchVector match;
		string tmpl_name;
		map<string, string> tmpl_params;
		int numbered_param;
		vector<string> params;
		vector<string> name_value;
		string param_name;
		string param_value;

		string data = origdata;
		COMMENT_REGEX.replace(&data, ""); // Strip comments
		NOWIKI_REGEX.replace(&data, ""); // Strip nowiki
		BR_REGEX.replace(&data, " "); // Replace BR

		while (match_found) {
			if (++itercnt > MAX_ITERATIONS) {
				return;
			}

			match_found = _getTemplates(&data, &markers, &templates, 0, data.length());
		}

		// Parse the template names and parameters
		for (auto templ : templates) {
			PhpPreg& template_regex = regexs["template"];
			template_regex.match(templ, &match);
			tmpl_name = match["name"]->text;

			// Replace any markers in the name
			MARKER_REGEX.matchAll(tmpl_name, &marker_matches);
			for (auto &marker_match : marker_matches) {
				string_replace(&tmpl_name, marker_match->at(0)->text, markers[marker_match->at(0)->text]);
			}

			string_replace(&tmpl_name, "_", " ");
			string_trim(&tmpl_name);
			tmpl_name[0] = toupper(tmpl_name[0]);
			if (tmpl_name.find("Template:") == 0) {
				tmpl_name.erase(0, 9);
				string_trim(&tmpl_name);
				tmpl_name[0] = toupper(tmpl_name[0]);
			}

			tmpl_params.clear();
			if (match.isSet("params")) {
				numbered_param = 1;
				string_split(match["params"]->text, "|", &params);

				for (auto param : params) {
					if (param.find('=') != string::npos) {
						string_split(param, "=", &name_value, 2);
						param_name = name_value[0];
						param_value = name_value[1];

						if (param_name.length() && param_name.back() == '\n') { // = must be on same line as param name
							param_name = to_string(numbered_param);
							param_value = param;
							++numbered_param;

						} else {
							// Replace any markers in the name
							MARKER_REGEX.matchAll(param_name, &marker_matches);
							for (auto &marker_match : marker_matches) {
								string_replace(&param_name, marker_match->at(0)->text, markers[marker_match->at(0)->text]);
							}
						}
					} else {
						param_name = to_string(numbered_param);
						param_value = param;
						++numbered_param;
					}

					// Replace any markers in the content
					MARKER_REGEX.matchAll(param_value, &marker_matches);
					for (auto &marker_match : marker_matches) {
						string_replace(&param_value, marker_match->at(0)->text, markers[marker_match->at(0)->text]);
					}

					string_trim(&param_name);
					string_trim(&param_value);
					if (param_name.length()) tmpl_params[param_name] = param_value;
				}
			}

			results->emplace_back(tmpl_name, tmpl_params);
		}
}

bool MWTemplateParamParser::_getTemplates(string *data, map<string, string> *markers, vector<string> *templates, int start, int length)
{
	int match_cnt;
	int offset_adjust;
	vector<shared_ptr<MatchVector>> matches;
	string marker_id;
	int content_len;
	int offset;
	vector<shared_ptr<MatchVector>> marker_matches;
	bool match_found;
	string substrdata;

	substrdata = data->substr(start, length);

	for (auto &regexname : regexs_ordered) {
		PhpPreg& type_regex = regexs[regexname];
		match_cnt = type_regex.matchAll(substrdata, &matches);
		offset_adjust = 0;

		if (match_cnt) {

			for (auto match : matches) {
				// See if there are any containers inside
				match_found = _getTemplates(data, markers, templates, start + match->at("content")->textOffset - offset_adjust,
					match->at("content")->text.length());
                if (match_found) return true; // Restart because data changed

				// Replace the match with a marker
				ostringstream oss;
				oss << "\x02" << markers->size() << "\x03";
				marker_id = oss.str();
				string content = match->at(0)->text;
				content_len = content.length();
				offset = start + match->at(0)->textOffset - offset_adjust;
				offset_adjust += content_len - marker_id.length();

				data->replace(offset, content_len, marker_id);

				if (regexname == "template") templates->push_back(content);

				// Replace any markers in the content
				MARKER_REGEX.matchAll(content, &marker_matches);
				for (auto &marker_match : marker_matches) {
					string_replace(&content, marker_match->at(0)->text, (*markers)[marker_match->at(0)->text]);
				}

				(*markers)[marker_id] = content;
			}

			return true; // Restart because data changed
		}
	}

    return false;
}


} /* namespace phppreg */
