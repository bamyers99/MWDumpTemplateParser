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

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string.h>
#include <memory>
#include <sstream>
#include <set>
#include "PregMatch.h"
#include "PhpPreg.h"
#include "MWDumpHandler.h"
#include "MWTemplateParamParser.h"
#include "MWTemplate.h"
#include "string_util.h"
#include <expat.h>

using namespace std;
using namespace phppreg;

int performTests();
int calcOffsets(string infilepath, string outfilepath);
int dumpValues(string infilepath, string outfilepath, string templatenames, bool verbose);

/**
 * Sample usage:
 * bunzip2 -c *pages-articles.xml.bz2 | ./MWDumpTemplateParser -v - enwikiTemplateParams enwikiTemplateTotals&
 * LC_ALL=C sort -n -k 1,1 -k 2,2 enwikiTemplateParams >enwikiTemplateParams.sorted
 * ./MWDumpTemplateParser -offsets enwikiTemplateParams.sorted enwikiTemplateOffsets
 *
 * bunzip2 -c *pages-articles.xml.bz2 | ./MWDumpTemplateParser -v -values - enwiki "IMDb name;IMDB name"&
 */

class TemplateInfo
{
public:
	int pagecount = 0;
	int instancecount = 0;
	map<string, int> param_name_cnt;
	map<string, map<string, int>> param_value_cnt;
	map<string, char> param_valid;
};

class MainClass : IPageHandler
{
public:
	int parseTemplates(const string& infilepath, const string& outfilepath, const string& totalsoutfilepath);
	void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data, const std::string& page_title);
	void loadTemplateIds();
	void writeTotals(const string& totalsoutfilepath);
	bool verbose = false;
	map<string, int> template_ids;
    ostream *dest = 0;
    map<int, TemplateInfo *> template_info;
    static map<int, bool> blacklist;
};

map<int, bool> MainClass::blacklist = {
    	{7585648, true},  // Reflist
		{21067859, true}, // Italic title
		{945764, true},   // Hatnote
		{1239772, true},  // Clear
		{3218295, true},  // Col-begin
		{3218301, true},  // Col-end
    	{4148498, true},  // Cite web
		{4321630, true},  // Cite news
		{4086375, true},  // Cite book
		{8387047, true},  // Convert
		{3501055, true},  // Persondata
		{4740319, true},  // Cite journal
		{10118245, true}, // Coord
		{23092408, true}, // Sfn
		{3164016, true},  // Citation
		{8577339, true},  // Dts
		{689990, true},   // Taxobox
		{2868018, true},  // Nihongo
		{5326834, true},  // Harvard citation no brackets
		{22612844, true}, // Football box collapsible
		{5098428, true},  // Episode list
		{9731674, true},  // Jct
		{6075367, true},  // FlagIOCathlete
		{10314667, true}, // Sortname
		{23470924, true}, // FlagIOC2athlete
		{11850048, true}, // Location map~
		{11849914, true}, // Location map+
		{30759408, true}, // NRHP row
		{1794726, true},  // Election box turnout
		{10109193, true}, // CBB schedule entry
		{3544598, true},  // Football box
		{2468023, true},  // About
		{8579816, true},  // CFB Schedule Entry
		{6664883, true},  // Goal
		{24576447, true}, // Singlechart
		{1794728, true},  // Election box majority
		{34231814, true}, // Canadian election result
		{7127202, true},  // USS
		{19963792, true}, // Episode list/sublist
		{10840750, true}, // HMS
		{11359357, true}, // Extended football squad player
		{3969498, true},  // National football squad player
		{24724613, true}, // IPAc-pl
		{7486256, true},  // Infobox NRHP
		{1807946, true},  // Election box hold with party link
		{41251413, true}, // Vcite2 journal
		{12077928, true}, // Fb cl team
		{20030541, true}, // Fb cl2 team
		{2075417, true},  // Flagicon
		{1695548, true},  // Flag
		{2048472, true},  // Citation needed
		{134352, true},   // Infobox football biography
		{3029961, true},  // Football squad player
		{1406921, true},  // Infobox settlement
		{3382507, true},  // Infobox person
		{21044097, true}  // Use dmy dates
    };


int main(int argc, char **argv) {
	int i;
	bool testmode = false;
	bool verbose = false;
	bool calcoffsets = false;
	bool dumpvalues = false;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-v") == 0) verbose = true;
		else if (strcmp(argv[i], "-t") == 0) testmode = true;
		else if (strcmp(argv[i], "-offsets") == 0) calcoffsets = true;
		else if (strcmp(argv[i], "-values") == 0) dumpvalues = true;
		else break;
	}

	if ((! calcoffsets && argc - i != 3) || (calcoffsets && argc - i != 2)) {
		cout << "Usage: MWDumpTemplateParser [-v] [-t] [-offsets] [infilepath|-] [outfilepath|-] [totals outfilepath|values template name(s)|-]\n";
		cout << "\t -v: verbose\n";
		cout << "\t -t: testmode\n";
		cout << "\t -offsets: calc template start offsets\n";
		cout << "\t -values: dump template parameter values\n";
		cout << "\t [infilepath|-]: input file path or - for stdin\n";
		cout << "\t [outfilepath|-]: output file path or - for stdout\n";
		cout << "\t [totals outfilepath|values template name(s)|-]: totals output file path or values template name(s) (separated by ;) or - for stderr\n";
		return 1;
	}

	string infilepath = argv[i];
	string outfilepath = argv[i+1];
	string totalsoutfilepath;
	if (! calcoffsets) totalsoutfilepath = argv[i+2];

	if (testmode) {
		return performTests();
	} else if (calcoffsets) {
		return calcOffsets(infilepath, outfilepath);
	} else if (dumpvalues) {
		return dumpValues(infilepath, outfilepath, totalsoutfilepath, verbose);
	}

	MainClass mc;
	mc.verbose = verbose;
	mc.loadTemplateIds();
	return mc.parseTemplates(infilepath, outfilepath, totalsoutfilepath);
}

int performTests()
{
	cout << "Performing tests\n";

	/**
	 * MatchVector tests
	 */

	// MatchVector.addItem
	MatchVector testVector;
	string testitem1text = "Test item 1";
	int testitem1textOffset = 5;
	int matchid = testVector.addItem(testitem1textOffset, testitem1text);
	if (matchid != 0) {
		cout << "MatchVector.addItem failed\n";
		return 1;
	}

	// MatchVector.setName invalid vectorOffset
	try {
		testVector.setName(1, "fail");
		cout << "MatchVector.setName invalid vectorOffset failed\n";
		return 2;
	} catch (const out_of_range& e) {
		// NOP
	}

	// MatchVector.setName
	testVector.setName(0, "firstcapture");

	// MatchVector.operator[]
	shared_ptr<MatchItem> item1 = testVector["firstcapture"];
	if (testitem1text != item1->text) {
		cout << "MatchVector.operator[] failed\n";
		return 3;
	}

	// Instance survival
	testVector.clear();
	if (item1->textOffset != testitem1textOffset) {
		cout << "instance survival failed\n";
		return 4;
	}

	/**
	 * PhpPreg tests
	 */

	// Missing ending delimiter
	PhpPreg phpPreg1("/abc");
	if (! phpPreg1.isError()) {
		cout << "Missing ending delimiter failed\n";
		return 5;
	}

	// Invalid modifier
	PhpPreg phpPreg2("/abc/Z");
	if (! phpPreg2.isError()) {
		cout << "Invalid modifier failed\n";
		return 6;
	}

	//Compile error
	PhpPreg phpPreg3("/[/");
	if (! phpPreg3.isError()) {
		cout << "Compile error failed\n";
		return 7;
	}

	// Named offset
	PhpPreg phpPreg4("!\\[\\[(?P<content>[^\\[\\]]*?)\\]\\]!");
	MatchVector mv;
	int matchcnt = phpPreg4.match("Planet [[earth]] is home", &mv);
	if (! matchcnt || phpPreg4.isError()) {
		cout << "Named offset match count failed\n";
		return 8;
	}

	shared_ptr<MatchItem> mi = mv["content"];

	if (mi->text != "earth" || mi->textOffset != 9) {
		cout << "Named offset text/textOffset failed\n";
		return 9;
	}

	// Numeric offset
	mi = mv[0];
	if (mi->text != "[[earth]]" || mi->textOffset != 7) {
		cout << "Numeric offset text/textOffset failed\n";
		return 10;
	}

	// Invalid numeric offset
	try {
		mi = mv[20];
		cout << "Invalid numeric offset failed\n";
		return 11;
	} catch (const out_of_range& e) {
		// NOP
	}

	// isSet
	if (! mv.isSet("content")) {
		cout << "isSet true failed\n";
		return 12;
	}

	if (mv.isSet("notfound")) {
		cout << "isSet false failed\n";
		return 13;
	}

	// matchAll
	PhpPreg phpPreg5("!a(b(?:c|d))!");
	vector<shared_ptr<MatchVector>> mvs;
	matchcnt = phpPreg5.matchAll("abc abd", &mvs);
	if (matchcnt != 2 || phpPreg5.isError()) {
		cout << "matchAll failed\n";
		return 14;
	}

	if (mvs.size() != 2) {
		cout << "matchAll size failed\n";
		return 15;
	}

	if (mvs[0]->at(0)->text != "abc" || mvs[0]->at(0)->textOffset != 0) {
		cout << "matchAll[0] whole match failed\n";
		return 16;
	}

	if (mvs[0]->at(1)->text != "bc" || mvs[0]->at(1)->textOffset != 1) {
		cout << "matchAll[0] capture match failed\n";
		return 17;
	}

	if (mvs[1]->at(0)->text != "abd" || mvs[1]->at(0)->textOffset != 4) {
		cout << "matchAll[1] whole match failed\n";
		return 18;
	}

	if (mvs[1]->at(1)->text != "bd" || mvs[1]->at(1)->textOffset != 5) {
		cout << "matchAll[1] capture match failed\n";
		return 19;
	}

	// replace
	PhpPreg phpPreg6("/<!--.*?-->/us");
	string subject = "begin<!-- some comment --> end";
	phpPreg6.replace(&subject, "");
	if (subject != "begin end") {
		cout << "PhpPreg::replace failed\n";
		return 20;
	}

	/**
	 * string_util tests
	 */

	// string_replace
	subject = "do you know your abc's? abc's yes";
	string_replace(&subject, "abc", "def");
	if (subject != "do you know your def's? def's yes") {
		cout << "string_replace failed\n";
		return 21;
	}

	// string_trim
	subject ="\t non-whitespace \n non-whitespace\r\n";
	string_trim(&subject);
	if (subject != "non-whitespace \n non-whitespace") {
		cout << "string_trim failed\n";
		return 22;
	}

	// string_split
	subject ="abc|def|hij";
	vector<string> splits;
	string_split(subject, "|", &splits);
	if (splits.size() != 3 || splits[0] != "abc" || splits[1] != "def" || splits[2] != "hij") {
		cout << "string_split failed\n";
		return 23;
	}

	subject ="ABC";
	string_split(subject, "|", &splits);
	if (splits.size() != 1 || splits[0] != "ABC") {
		cout << "string_split sep not found failed\n";
		return 24;
	}

	/**
	 * MWTemplateParamParser
	 */

	// getTemplates
	vector<MWTemplate> results;
	string origdata = " \
		{{Nihongo|Cindy Aurum|シドニー・オールム|Shidonī Ōrumu|'Cidney'<ref name= 'SilMoogle'/>}} \
		{{Infobox_person \
		|name = [[Fred]] <!-- some comment --> \
		|birth_date={{birth date|1984|12|13}} \
		}} \
		It is true.<ref>{{Cite web|url=http://a.com|title=Website}}</ref> \
		{| \
		|- \
		|abc {{{2}}} {{sort|ABC}} \
		|} \
		";
	MWTemplateParamParser::getTemplates(&results, origdata);

	if (results.size() != 5) {
		cout << "MWTemplateParamParser::getTemplates failed\n";
		return 25;
	}

	for (auto &tmpl : results) {
		if (tmpl.name == "Infobox person") {
			if (tmpl.params["name"] != "[[Fred]]" || tmpl.params["birth_date"] != "{{birth date|1984|12|13}}") {
				cout << "Template Infobox person failed\n";
				return 26;
			}

		} else if (tmpl.name == "Birth date") {
			if (tmpl.params["1"] != "1984" || tmpl.params["2"] != "12" || tmpl.params["3"] != "13") {
				cout << "Template Birth date failed\n";
				return 27;
			}

		} else if (tmpl.name == "Cite web") {
			if (tmpl.params["url"] != "http://a.com" || tmpl.params["title"] != "Website") {
				cout << "Template Cite web failed\n";
				return 28;
			}

		} else if (tmpl.name == "Sort") {
			if (tmpl.params["1"] != "ABC") {
				cout << "Template Sort failed\n";
				return 29;
			}

		} else if (tmpl.name == "Nihongo") {
			if (tmpl.params["1"] != "Cindy Aurum" || tmpl.params["2"] != "シドニー・オールム" || tmpl.params["3"] != "Shidonī Ōrumu"
					|| tmpl.params["4"] != "'Cidney'<ref name= 'SilMoogle'/>") {
				cout << "Template Nihongo failed\n";
				return 30;
			}

		} else {
			cout << "MWTemplateParamParser::getTemplates failed unknown tmpl.name = " << tmpl.name << "\n";
			return 31;
		}
	}

	MainClass mc;
	string infilepath = "MWDumpTest.xml";
	string outfilepath = "-";
	string totalsoutfilepath = "-";
	mc.loadTemplateIds();
	int retval = mc.parseTemplates(infilepath, outfilepath, totalsoutfilepath);
	if (retval) {
		cout << "parseTemplates failed = " << retval << "\n";
		return 32;
	}

	/**
	 * Calc offset test
	 */

	infilepath = "CalcOffsetTest.tsv";
	outfilepath = "-";
	retval = calcOffsets(infilepath, outfilepath);
	if (retval) {
		cout << "calcOffsets failed = " << retval << "\n";
		return 33;
	}

	/**
	 * processPage() test
	 */
	string pagedata = " \
			{{Infobox musical artist\
| name = Gianluca Grignani\
| image = Gianluca Grignani - Fonte Nuova 31-05-08.JPG\
| caption = Gianluca Grignani (2008)\
| background = solo_singer\
| birth_date = {{birth date and age|1972|04|07|df=y}}\
| origin= [[Milan]], Italy\
| instrument = Vocals, guitar, piano\
| genre = [[Pop music|Pop]]\
| occupation = Singer-songwriter, guitarist\
| years_active = 1994–present\
| First album = \
| Latest album = \
| Notable albums = \
| Notable songs = \
| Label = [[Mercury Records]], [[Ils International]], [[Strategic Marketing]], [[PolyGram]], [[Universal Music]], [[Balboa Recording Corporation]], [[Columbia Records]], [[Sony Music]]\
\
| website = {{URL|http://www.grignani.it}}\
}}";

	mc.dest = new ostringstream();
	mc.processPage(0, 113, 1, pagedata, "Gianluca Grignani");
	string output = ((ostringstream *)mc.dest)->str();
	delete mc.dest;

	string_split(output, "\t", &splits);

	if (splits[0] != "4592538" || splits[1] != "113") {
		cout << "processPage failed, wrong template id or page id\n";
		return 34;
	}

	map<string, string> params;
	for (unsigned int x=2; x < splits.size(); x += 2) {
		params[splits[x]] = splits[x+1];
	}

	map<string, bool> paramnames = {{"name",true},{"image",true},{"caption",true},{"background",true},{"birth_date",true},
			{"origin",true},{"instrument",true},{"genre",true},{"occupation",true},{"years_active",true},{"First album",false},
			{"Latest album",false},{"Notable albums",false},{"Notable songs",false},{"Label",true},{"website",true}};

	for (auto param : paramnames) {
		auto it = params.find(param.first);
		if (param.second && it == params.end()) {
			cout << "processPage failed, missing param " << param.first <<"\n";
			return 35;
		}
		if (! param.second && it != params.end()) {
			cout << "processPage failed, extra param " << param.first <<"\n";
			return 36;
		}
	}

	/**
	 * Misc test
	 */

	pagedata = R"END(
{{Infobox settlement
|official_name =Junabad
|native_name =جون اباد
|settlement_type        = village
|coordinates_region     = IR
|subdivision_type       = [[List of countries|Country]]
|subdivision_name = {{flag|Iran}}
|subdivision_type1 =[[Provinces of Iran|Province]]
|subdivision_name1 =[[Fars Province|Fars]]
|subdivision_type2 =[[Counties of Iran|County]]
|subdivision_name2 = [[Fasa County|Fasa]]
|subdivision_type3 =[[Bakhsh]]
|subdivision_name3 =[[Central District (Fasa County)|Central]]
|subdivision_type4 =[[Rural Districts of Iran|Rural District]]
|subdivision_name4 =[[Sahrarud Rural District|Sahrarud]]
|leader_title           = 
|leader_name            = 
|established_title      =
|established_date       = 
|area_total_km2           = 
|area_footnotes           = 
|population_as_of         = 2006
|population_total =
|population_density_km2   =auto
|timezone               = [[Iran Standard Time|IRST]]
|utc_offset             = +3:30
|timezone_DST           = [[Iran Daylight Time|IRDT]]
|utc_offset_DST         = +4:30
|coordinates_display      = %
|latd=|latm=|lats=|latNS=N
|longd=|longm=|longs=|longEW=E
|elevation_m            = 
|area_code              = 
|website                = 
|footnotes              =
}}
'''Junabad''' ({{lang-fa|جون اباد}}, also [[Romanize]]d as '''Jūnābād''') is a village in [[Sahrarud Rural District]], in the [[Central District (Fasa County)|Central District]] of [[Fasa County]], [[Fars Province]], [[Iran]]. At the 2006 census, its existence was noted, but its population was not reported.<ref>{{IranCensus2006|07}}</ref>  

== References ==
{{reflist}}

{{Fasa County}}

{{coord missing|Iran}}

[[Category:Populated places in Fasa County]]

{{Fasa-geo-stub}}
)END";

	mc.dest = new ostringstream();
	mc.processPage(0, 50407944, 1, pagedata, "Junabad");
	output = ((ostringstream *)mc.dest)->str();
	delete mc.dest;

	vector<string> templates;
	string_split(output, "\n", &templates);

	for (auto tmpl : templates) {
		if (tmpl.length() == 0) continue;
		string_split(tmpl, "\t", &splits);
		cout << "Template id: " << splits[0] << "\n";

		for (unsigned int x=2; x < splits.size(); x += 2) {
			cout << splits[x] << " = " << splits[x+1] << "\n";
			params[splits[x]] = splits[x+1];
		}
	}

	/**
	 * dumpValues test
	 */

	infilepath = "MWDumpTest.xml";
	outfilepath = "-";
	retval = dumpValues(infilepath, outfilepath, "Infobox person;Infobox Person", true);
	if (retval) {
		cout << "dumpValues failed = " << retval << "\n";
		return 37;
	}

	cout << "All tests passed\n";
	return 0;
}

MWDumpHandler *mwdh;

void XMLCALL startElement(void *userData, const char *el, const char **attr)
{
	mwdh->startElement(userData, el, attr);
}

void XMLCALL endElement(void *userData, const char *el)
{
	mwdh->endElement(userData, el);
}

void XMLCALL characters(void *userData, const XML_Char *s, int len)
{
	mwdh->characters(userData, s, len);
}

int MainClass::parseTemplates(const string& infilepath, const string& outfilepath, const string& totalsoutfilepath)
{
	// Check for single byte xml characters for utf8 internal data
	const XML_Feature *fl = XML_GetFeatureList();
	if (fl->feature != XML_FEATURE_SIZEOF_XML_CHAR || fl->value != 1) {
		cerr << "expat not compiled for single byte xml characters\n";
		return 1;
	}

	XML_Parser p = XML_ParserCreate("UTF-8");
	if (! p) {
		cerr << "Couldn't allocate memory for parser\n";
	    return 2;
	}

	XML_SetElementHandler(p, startElement, endElement);
	XML_SetCharacterDataHandler(p, characters);

    MWDumpHandler defaultHandler(*this);
    mwdh = &defaultHandler;

    istream *source;
    if (infilepath == "-") {
    	source = &cin;
    } else {
    	source = new ifstream(infilepath.c_str(), ios::in|ios::binary);
    	if (source->fail()) {
    	    cerr << "new ifstream failed for " << infilepath << "\n";
    	    return 3;
    	}
    }

    if (outfilepath == "-") {
    	dest = &cout;
    } else {
    	dest = new ofstream(outfilepath.c_str(), ios::out|ios::binary|ios::trunc);
    	if (dest->fail()) {
    	    cerr << "new ofstream failed for " << outfilepath << "\n";
    	    return 4;
    	}
    }

	int bytes_read;
	char *buff;

    for (;;) {
    	buff = (char *)XML_GetBuffer(p, 2048);
    	if (buff == NULL) {
    	    cerr << "XML_GetBuffer failed\n";
    	    return 5;
    	}

    	source->read(buff, 2048);
    	if (source->fail() && ! source->eof()) {
			cerr << "source->read failed\n";
			return 6;
    	}
    	bytes_read = source->gcount();

    	if (bytes_read >= 0) {
    		if (! XML_ParseBuffer(p, bytes_read, source->eof())) {
    			cerr << "XML_ParseBuffer failed\n";
    			return 7;
    		}
    	}

    	if (source->eof()) break;
    }

    XML_ParserFree(p);

    if (infilepath != "-") delete source;
    if (outfilepath != "-") delete dest;

    writeTotals(totalsoutfilepath);

	return 0;
}

void MainClass::processPage(int ns, unsigned int page_id, unsigned int revid, const std::string& page_data, const std::string& page_title)
{
	static int pagecnt = 0;

	if (ns != 0) return; // only want articles
	++pagecnt;
	if (pagecnt % 100000 == 0 && verbose) cerr << pagecnt << "\n";

	// Parse the templates
	vector<MWTemplate> templates;
	MWTemplateParamParser::getTemplates(&templates, page_data);
	int tmplid;
	map<int, int> pagetemplates;
	bool blacklisted;

	for (auto &templ : templates) {
		if (template_ids.find(templ.name) == template_ids.end()) continue;
		tmplid = template_ids[templ.name];

		// Fancy code to erase map elements while iterating the map
		for (map<string,string>::iterator it=templ.params.begin(), it_next=it, it_end=templ.params.end();
		    it != it_end;
		    it = it_next)
		{
			++it_next;
			if (it->second.length() == 0) templ.params.erase(it);
		}

		if (templ.params.empty()) continue;

		++pagetemplates[tmplid];

		if (pagetemplates[tmplid] == 1) ++template_info[tmplid]->pagecount;
		++template_info[tmplid]->instancecount;

		blacklisted = (blacklist.find(tmplid) != blacklist.end());
		bool writeblacklisted = false;

		// Determine if blacklisted template needs to be written out: invalid/deprecated/required/suggested param
		if (blacklisted) {
			// invalid
			for (auto &pair : templ.params) {
				string key = pair.first;
				if (template_info[tmplid]->param_valid.find(key) == template_info[tmplid]->param_valid.end()) {
					writeblacklisted = true;
					break;
				}
			}

			// deprecated/required/suggested
			if (! writeblacklisted) {
				for (auto &pair : template_info[tmplid]->param_valid) {
					string key = pair.first;
					char value = pair.second;

					if (value == 'D' && templ.params.find(key) != templ.params.end()) {
						writeblacklisted = true;
						break;
					}

					if ((value == 'R' || value == 'S') && templ.params.find(key) == templ.params.end()) {
						writeblacklisted = true;
						break;
					}
				}
			}
		}

		if (! blacklisted || writeblacklisted) *dest << tmplid << "\t" << page_id;

		for (auto &pair : templ.params) {
			string key = pair.first;
			string& value = pair.second;
			for (auto &achar : key) if (achar == '\n' || achar == '\t') achar = ' '; // Don't want tabs/newlines in csv file
			for (auto &achar : value) if (achar == '\n' || achar == '\t') achar = ' ';
			if (key.length() > 255) key.erase(255);
			if (value.length() > 255) value.erase(255);

			// Calc unique values
			++template_info[tmplid]->param_name_cnt[key];

			if (template_info[tmplid]->param_value_cnt[key].size() == 50) {
				if (! blacklisted || writeblacklisted) *dest << "\t" << key << "\t"; // Don't write the value out, need key for templates having 'key' searches
			} else {
				++template_info[tmplid]->param_value_cnt[key][value];
				if (! blacklisted) *dest << "\t" << key << "\t" << value;
				else if (writeblacklisted) *dest << "\t" << key << "\t";
			}
		}

		if (! blacklisted || writeblacklisted) *dest << "\n";
	}
}

void MainClass::loadTemplateIds()
{
	string infilepath = "TemplateIds.tsv";
	ifstream source(infilepath.c_str(), ios::in|ios::binary);
	if (source.fail()) {
	    cerr << "new ifstream failed for " << infilepath << "\n";
	    return;
	}

	string line;
	vector<string> pieces;

	while (source.good()) {
		getline(source, line);

		if (line.length()) {
			string_split(line, "\t", &pieces);
			string name(pieces[0]);
			int id = stoi(pieces[1]);
			template_ids[name] = id;
			if (template_info.find(id) == template_info.end()) template_info[id] = new TemplateInfo();

			// Load the parameter names/validity
			if (pieces.size() > 2) {
				for (unsigned int i = 2; i < pieces.size(); i += 2) {
					char validity = pieces[i + 1][0];
					template_info[id]->param_valid[pieces[i]] = validity;
				}
			}
		}
	}
}

void MainClass::writeTotals(const string& totalsoutfilepath)
{
    ostream *dest;
    if (totalsoutfilepath == "-") {
    	dest = &cerr;
    } else {
    	dest = new ofstream(totalsoutfilepath.c_str(), ios::out|ios::binary|ios::trunc);
    	if (dest->fail()) {
    	    cerr << "new ofstream failed for " << totalsoutfilepath << "\n";
    	}
    }

    for (auto &info_pair : template_info) {
    	TemplateInfo* ti = info_pair.second;
    	if (ti->pagecount == 0) continue;

    	*dest << "T" << info_pair.first << "\t" << ti->pagecount << "\t" << ti->instancecount << "\n";

    	for (auto &param_pair : ti->param_name_cnt) {
    		const string& param_name = param_pair.first;

    		*dest << "P" << param_name << "\t" << param_pair.second;

    		if (ti->param_value_cnt[param_name].size() != 50) {
    			for (auto &value_pair : ti->param_value_cnt[param_name]) {
    				*dest << "\t" << value_pair.first << "\t" << value_pair.second;
    			}
    		}

        	*dest << "\n";
    	}
    }

    if (totalsoutfilepath != "-") delete dest;
}

int calcOffsets(string infilepath, string outfilepath)
{
	bool blacklisted;
    istream *source;
    if (infilepath == "-") {
    	source = &cin;
    } else {
    	source = new ifstream(infilepath.c_str(), ios::in|ios::binary);
    	if (source->fail()) {
    	    cerr << "new ifstream failed for " << infilepath << "\n";
    	    return 1;
    	}
    }

    ostream *dest;
    if (outfilepath == "-") {
    	dest = &cout;
    } else {
    	dest = new ofstream(outfilepath.c_str(), ios::out|ios::binary|ios::trunc);
    	if (dest->fail()) {
    	    cerr << "new ofstream failed for " << outfilepath << "\n";
    	    return 2;
    	}
    }

    string prevTemplID;
	string line;
	vector<string> pieces;

	while (source->good()) {
		long long offset = source->tellg();

		getline(*source, line);

		if (line.length()) {
			string_split(line, "\t", &pieces);
			string templID(pieces[0]);

			if (templID != prevTemplID) {
				int tmplid = atoi(templID.c_str());
				blacklisted = (MainClass::blacklist.find(tmplid) != MainClass::blacklist.end());
				string blsign;
				if (blacklisted) blsign = "-";

				*dest << templID << "\t" << blsign << offset << "\n";
			}

			prevTemplID = templID;
		}
	}

    if (outfilepath != "-") delete dest;

	return 0;
}

class ValuesHandler : public IPageHandler
{
public:
	bool verbose;
	set<string> templatenames;
	void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data, const std::string& page_title);
	map<string, map<string, map<string, string>>> param_values; // pagename, template name, parameter name, parameter value
};

void ValuesHandler::processPage(int ns, unsigned int page_id, unsigned int revid, const std::string& page_data, const std::string& page_title)
{
	static int pagecnt = 0;

	if (ns != 0) return; // only want articles
	++pagecnt;
	if (pagecnt % 100000 == 0 && verbose) cerr << pagecnt << "\n";

	// Parse the templates
	vector<MWTemplate> templates;
	MWTemplateParamParser::getTemplates(&templates, page_data);
	map<string, int> pagetemplates;

	for (auto &templ : templates) {
		if (templatenames.find(templ.name) == templatenames.end()) continue;

		// Fancy code to erase map elements while iterating the map
		for (map<string,string>::iterator it=templ.params.begin(), it_next=it, it_end=templ.params.end();
		    it != it_end;
		    it = it_next)
		{
			++it_next;
			if (it->second.length() == 0) templ.params.erase(it);
		}

		if (templ.params.empty()) continue;

		++pagetemplates[templ.name];
		string temptmplname = templ.name;

		if (pagetemplates[templ.name] > 1) temptmplname += "{" + to_string(pagetemplates[templ.name]) + "}";

		for (auto &pair : templ.params) {
			string key = pair.first;
			string& value = pair.second;
			for (auto &achar : key) if (achar == '\n' || achar == '\t') achar = ' '; // Don't want tabs/newlines in csv file
			for (auto &achar : value) if (achar == '\n' || achar == '\t') achar = ' ';
			if (key.length() > 255) key.erase(255);
			if (value.length() > 255) value.erase(255);

			param_values[page_title][temptmplname][key] = value;
		}
	}

}

int dumpValues(string infilepath, string outfilepath, string templatenames, bool verbose)
{
	// Check for single byte xml characters for utf8 internal data
	const XML_Feature *fl = XML_GetFeatureList();
	if (fl->feature != XML_FEATURE_SIZEOF_XML_CHAR || fl->value != 1) {
		cerr << "expat not compiled for single byte xml characters\n";
		return 1;
	}

	XML_Parser p = XML_ParserCreate("UTF-8");
	if (! p) {
		cerr << "Couldn't allocate memory for parser\n";
	    return 2;
	}

	XML_SetElementHandler(p, startElement, endElement);
	XML_SetCharacterDataHandler(p, characters);

	vector<string> temptemplates;
	string_split(templatenames, ";", &temptemplates);
	string maintemplate = temptemplates[0];
	set<string> templates(temptemplates.begin(), temptemplates.end());

	ValuesHandler vh;
	vh.verbose = verbose;
	vh.templatenames = templates;
    MWDumpHandler defaultHandler(vh);
    mwdh = &defaultHandler;

    istream *source;
    if (infilepath == "-") {
    	source = &cin;
    } else {
    	source = new ifstream(infilepath.c_str(), ios::in|ios::binary);
    	if (source->fail()) {
    	    cerr << "new ifstream failed for " << infilepath << "\n";
    	    return 3;
    	}
    }

	int bytes_read;
	char *buff;

    for (;;) {
    	buff = (char *)XML_GetBuffer(p, 2048);
    	if (buff == NULL) {
    	    cerr << "XML_GetBuffer failed\n";
    	    return 5;
    	}

    	source->read(buff, 2048);
    	if (source->fail() && ! source->eof()) {
			cerr << "source->read failed\n";
			return 6;
    	}
    	bytes_read = source->gcount();

    	if (bytes_read >= 0) {
    		if (! XML_ParseBuffer(p, bytes_read, source->eof())) {
    			cerr << "XML_ParseBuffer failed\n";
    			return 7;
    		}
    	}

    	if (source->eof()) break;
    }

    XML_ParserFree(p);

    ostream *dest;
    if (outfilepath == "-") {
    	dest = &cout;
    } else {
    	string outfilename = maintemplate;
    	string_replace(&outfilename, " ", "_");

    	dest = new ofstream(outfilepath + "_" + outfilename + ".tsv", ios::out|ios::binary|ios::trunc);
    	if (dest->fail()) {
    	    cerr << "new ofstream failed for " << outfilepath << "\n";
    	    return 4;
    	}
    }

	// Determine the parameter names used
	set<string> paramnames;
	for (auto &pair : vh.param_values) {
		for (auto &pair2 : pair.second) {
			for (auto &pair3 : pair2.second) {
				paramnames.insert(pair3.first);
			}
		}
	}

    // write the header
    *dest << "pagename" << "\t" << "templatename";

    for (auto &paramname : paramnames) {
    	*dest << "\t" << paramname;
    }

    *dest << "\n";

	for (auto &pair : vh.param_values) {
		string pagename = pair.first;

		for (auto &pair2 : pair.second) {
			string temptmplname = pair2.first;
			// strip occurrence number
			PhpPreg phpPreg("!^([^{]+)(?:\\{\\d+\\})?$!");
			MatchVector mv;
			phpPreg.match(temptmplname, &mv);
			temptmplname = mv[1]->text;

			*dest << pagename << "\t" << temptmplname;

			for (auto &paramname : paramnames) {
				string paramvalue;
				if (pair2.second.find(paramname) == pair2.second.end()) paramvalue = "";
				else paramvalue = pair2.second[paramname];
				*dest << "\t" << paramvalue;
			}

			*dest << "\n";
		}
	}

    if (infilepath != "-") delete source;
    if (outfilepath != "-") delete dest;

	return 0;
}
