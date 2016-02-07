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
#include "PregMatch.h"
#include "PhpPreg.h"
#include "MWDumpHandler.h"
#include "MWTemplateParamParser.h"
#include "MWTemplate.h"
#include <expat.h>

using namespace std;
using namespace phppreg;

int performTests();

class MainClass : IPageHandler
{
public:
	int parseTemplates(const string& infilepath, const string& outfilepath);
	void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data);
	bool verbose = false;
	map<string, int> template_ids;
    ostream *dest = 0;
};

int main(int argc, char **argv) {
	int i;
	bool testmode = false;
	bool verbose = false;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-v") == 0) verbose = true;
		else if (strcmp(argv[i], "-t") == 0) testmode = true;
		else break;
	}

	if (argc - i != 2) {
		cout << "Usage: MWDumpTemplateParser [-v] [-t] [infilepath|-] [outfilepath|-]\n";
		cout << "\t -v: verbose\n";
		cout << "\t -t: testmode\n";
		cout << "\t [infilepath|-]: input file path or - for stdin\n";
		cout << "\t [outfilepath|-]: output file path or - for stdout\n";
		return 1;
	}

	string infilepath = argv[i];
	string outfilepath = argv[i+1];

	if (testmode) {
		return performTests();
	}

	MainClass mc;
	mc.verbose = verbose;
	return mc.parseTemplates(infilepath, outfilepath);
}

int performTests()
{
	cout << "Performing tests\n";

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

int MainClass::parseTemplates(const string& infilepath, const string& outfilepath)
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
    }

    if (outfilepath == "-") {
    	dest = &cout;
    } else {
    	dest = new ofstream(outfilepath.c_str(), ios::out|ios::binary|ios::trunc);
    }

	int bytes_read;
	char *buff;

    for (;;) {
    	buff = (char *)XML_GetBuffer(p, 2048);
    	if (buff == NULL) {
    	    cerr << "XML_GetBuffer failed\n";
    	    return 3;
    	}

    	source->read(buff, 2048);
    	bytes_read = source->gcount();

    	if (bytes_read >= 0) {
    		if (! XML_ParseBuffer(p, bytes_read, source->eof())) {
    			cerr << "XML_ParseBuffer failed\n";
    			return 4;
    		}
    	}

    	if (source->eof()) break;
    }

    XML_ParserFree(p);

    if (infilepath != "-") delete source;
    if (outfilepath != "-") delete dest;

	return 0;
}

void MainClass::processPage(int ns, unsigned int page_id, unsigned int revid, const std::string& page_data)
{
	static int pagecnt = 0;

	if (ns != 0) return; // only want articles
	++pagecnt;
	if (pagecnt % 100000 == 0 && verbose) cerr << pagecnt << "\n";

	// Write the page id to the output file
	*dest << "P" << page_id << "\v" << revid << "\n";

	// Parse the templates
	vector<MWTemplate> templates;
	MWTemplateParamParser::getTemplates(&templates, page_data);
	map<int, int> pagetemplates;
	unsigned int tmplid;

	for (auto &templ : templates) {
		if (template_ids.find(templ.name) == template_ids.end()) continue;
		tmplid = template_ids[templ.name];

		for (auto const &pair : templ.params) {
			if (pair.second.length() == 0) templ.params.erase(pair.first);
		}

		if (templ.params.empty()) continue;

		*dest << "T" << tmplid;

		for (auto &pair : templ.params) {
			string key = pair.first;
			string& value = pair.second;
			for (auto &achar : value) if (achar == '\n') achar = '\f'; // Don't want newlines in csv file
			if (key.length() > 255) key.erase(255);
			if (value.length() > 255) value.erase(255);

			*dest << "\v" << key << "\v" << value;
		}

		*dest << "\n";
	}
}
