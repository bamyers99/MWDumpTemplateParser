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

#include "string_util.h"
#include <algorithm>
#include <cctype>
#include <vector>

using namespace std;

void string_replace(string *subject, const string& search, const string& sreplace, int limit)
{
    if (! limit || ! search.length()) return;
    string::size_type findPos = subject->find(search);
	if (findPos == string::npos) return;
	if (search.length() == 1 && sreplace.length() == 1) {
		replace(subject->begin(), subject->end(), search[0], sreplace[0]);
		return;
	}

	int rep_count = 0;
    int lastPos = 0;
    int searchLen = search.length();
    string newString;
    newString.reserve(subject->length());

    if (limit < 0) limit = 1000000;

    do {
        newString.append(*subject, lastPos, findPos - lastPos);
        newString += sreplace;
        lastPos = findPos + searchLen;
        if (++rep_count == limit) break;
        findPos = subject->find(search, lastPos);
    } while (findPos != string::npos);

    newString += subject->substr(lastPos);

    subject->swap(newString);
}

void string_trim(string *subject, const string& whitespace)
{
	string::size_type pos = subject->find_last_not_of(whitespace);
	if (pos == string::npos) {
		subject->clear();
		return;
	}
	++pos;
	if (pos < subject->length()) subject->erase(pos);

	pos = subject->find_first_not_of(whitespace);
	if (pos > 0) subject->erase(0, pos);
}

void string_split(const string& subject, const string& separator, vector<string> *pieces, int limit)
{
	pieces->clear();
	string::size_type findPos = subject.find(separator);
	if (findPos == string::npos) {
		pieces->emplace_back(subject);
		return;
	}
    int lastPos = 0;
    int sepLen = separator.length();
	int rep_count = 0;

    if (limit < 0) limit = 1000001;
    --limit;

	do {
		pieces->emplace_back(subject.substr(lastPos, findPos - lastPos));
        lastPos = findPos + sepLen;
        if (++rep_count == limit) break;
		findPos = subject.find(separator, lastPos);
	} while (findPos != string::npos);

	pieces->emplace_back(subject.substr(lastPos));
}
