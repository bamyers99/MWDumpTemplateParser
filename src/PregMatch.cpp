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

#include "PregMatch.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

using namespace std;

namespace phppreg {

/***************
 * MatchVector *
 ***************/
int MatchVector::addItem(const int textOffset, const string& text)
{
	emplace_back(new MatchItem(textOffset, text));
	return size() - 1;
}

int MatchVector::addItem(const int textOffset, const char* text)
{
	emplace_back(new MatchItem(textOffset, text));
	return size() - 1;
}

int MatchVector::addItem(const int textOffset, const unsigned char* text)
{
	emplace_back(new MatchItem(textOffset, text));
	return size() - 1;
}

int MatchVector::addItem(const int textOffset, const char* text, int length)
{
	emplace_back(new MatchItem(textOffset, text, length));
	return size() - 1;
}

int MatchVector::addItem(const int textOffset, const unsigned char* text, int length)
{
	emplace_back(new MatchItem(textOffset, text, length));
	return size() - 1;
}

void MatchVector::setName(const int vectorOffset, const string& text)
{
	if (vectorOffset < 0 || vectorOffset >= (int)size()) {
		ostringstream os;
		os << "MatchVector::setName invalid vectorOffset (" << vectorOffset << ")";
		throw out_of_range(os.str());
	}

	nameMap.emplace(text, vectorOffset);
}

void MatchVector::setName(const int vectorOffset, const char* text)
{
	if (vectorOffset < 0 || vectorOffset >= (int)size()) {
		ostringstream os;
		os << "MatchVector::setName invalid vectorOffset (" << vectorOffset << ")";
		throw out_of_range(os.str());
	}

	nameMap.emplace(text, vectorOffset);
}

void MatchVector::setName(const int vectorOffset, const unsigned char* text)
{
	if (vectorOffset < 0 || vectorOffset >= (int)size()) {
		ostringstream os;
		os << "MatchVector::setName invalid vectorOffset (" << vectorOffset << ")";
		throw out_of_range(os.str());
	}

	nameMap.emplace(reinterpret_cast<const char*>(text), vectorOffset);
}

void MatchVector::fillMap(const map<string, int> nameMap)
{
	this->nameMap = nameMap;
}

void MatchVector::clearMap()
{
	nameMap.clear();
}

const shared_ptr<MatchItem>& MatchVector::operator[] (const string index) const
{
	map<string, int>::const_iterator it = nameMap.find(index);
	if (it == nameMap.end()) {
		ostringstream os;
		os << "MatchVector::operator[] name (" << index << ") not found";
		throw out_of_range(os.str());
	}

	return at(it->second);
}

const shared_ptr<MatchItem>& MatchVector::operator[] (int index) const
{
	return at(index);
}

shared_ptr<MatchItem>& MatchVector::operator[] (const string index)
{
	map<string, int>::const_iterator it = nameMap.find(index);
	if (it == nameMap.end()) {
		ostringstream os;
		os << "MatchVector::operator[] name (" << index << ") not found";
		throw out_of_range(os.str());
	}

	return at(it->second);
}

shared_ptr<MatchItem>& MatchVector::operator[] (int index)
{
	return at(index);
}

bool MatchVector::isSet(const std::string index)
{
	return (nameMap.count(index) == 1);
}

} /* namespace phppreg */
