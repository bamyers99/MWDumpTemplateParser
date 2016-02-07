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

#ifndef PREGMATCH_H_
#define PREGMATCH_H_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <pcre.h>

namespace phppreg {

/*************
 * MatchItem *
 *************/
class PCRECPP_EXP_DEFN MatchItem
{
public:
	int textOffset;
	std::string text;

	MatchItem(const int textOffset, const std::string& text) : textOffset(textOffset), text(text) {}
	MatchItem(const int textOffset, const char* text) : textOffset(textOffset), text(text) {}
	MatchItem(const int textOffset, const unsigned char* text) : textOffset(textOffset), text(reinterpret_cast<const char*>(text)) {}
	MatchItem(const int textOffset, const char* text, int length) : textOffset(textOffset), text(text, length) {}
	MatchItem(const int textOffset, const unsigned char* text, int length) : textOffset(textOffset), text(reinterpret_cast<const char*>(text), length) {}

private:
	MatchItem() = delete;
};

/***************
 * MatchVector *
 ***************/
class PCRECPP_EXP_DEFN MatchVector : public std::vector<std::shared_ptr<MatchItem>>
{
public:
	MatchVector() : vector() {};
	int addItem(const int textOffset, const std::string& text);
	int addItem(const int textOffset, const char* text);
	int addItem(const int textOffset, const unsigned char* text);
	int addItem(const int textOffset, const char* text, int length);
	int addItem(const int textOffset, const unsigned char* text, int length);

	/**
	 * throws out_of_range if vectorOffset not found
	 */
	void setName(const int vectorOffset, const std::string& text);
	void setName(const int vectorOffset, const char* text);
	void setName(const int vectorOffset, const unsigned char* text);
	void fillMap(const std::map<std::string, int>& nameMap);
	void clearMap();

	/**
	 * throws out_of_range if name not found
	 */
	const std::shared_ptr<MatchItem>& operator[] (const std::string& index) const;
	const std::shared_ptr<MatchItem>& operator[] (int index) const;
	const std::shared_ptr<MatchItem>& at(const std::string& index) const;
	const std::shared_ptr<MatchItem>& at(int index) const;

	/**
	 * throws out_of_range if name not found
	 */
	std::shared_ptr<MatchItem>& operator[] (const std::string& index);
	std::shared_ptr<MatchItem>& operator[] (int index);
	std::shared_ptr<MatchItem>& at(const std::string& index);
	std::shared_ptr<MatchItem>& at(int index);

	/**
	 * isSet
	 *
	 * Test if a string index is set
	 *
	 * @param index Index to test
	 * @return Is it set
	 */
	bool isSet(const std::string& index);

protected:
	std::map<std::string, int> nameMap;
};

} /* namespace phppreg */

#endif /* PREGMATCH_H_ */
