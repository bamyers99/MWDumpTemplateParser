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
#include <string.h>
#include <memory>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/StdInInputSource.hpp>
#include "PregMatch.h"
#include "PhpPreg.h"
#include "MWDumpHandler.h"

using namespace std;
using namespace phppreg;
using namespace xercesc;

int performTests();

class MainClass : IPageHandler
{
public:
	int parseTemplates(const string& infilepath, const string& outfilepath);
	void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data);
	bool verbose = false;
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

int MainClass::parseTemplates(const string& infilepath, const string& outfilepath)
{
	XMLPlatformUtils::Initialize();

	unique_ptr<SAX2XMLReader> parser(XMLReaderFactory::createXMLReader());

    MWDumpHandler defaultHandler(*this);
    parser->setContentHandler(&defaultHandler);
    parser->setErrorHandler(&defaultHandler);

    unique_ptr<InputSource> source;
    if (infilepath == "-") {
    	source = unique_ptr<InputSource>(new StdInInputSource());
    } else {
    	XMLCh *tfp = XMLString::transcode(infilepath.c_str());
    	source = unique_ptr<InputSource>(new LocalFileInputSource(tfp));
    	XMLString::release(&tfp);
    }

    parser->parse(*source);

    XMLPlatformUtils::Terminate();

	return 0;
}

void MainClass::processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data)
{

}
