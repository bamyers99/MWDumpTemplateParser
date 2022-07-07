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

#ifndef MWTEMPLATEPARAMPARSER_H_
#define MWTEMPLATEPARAMPARSER_H_

#include <string>
#include <vector>
#include <map>
#include "MWTemplate.h"
#include "PhpPreg.h"

namespace phppreg {

class MWTemplateParamParser
{
public:
	MWTemplateParamParser() {}
	static void getTemplates(std::vector<MWTemplate> *templates, const std::string& origdata);
	virtual ~MWTemplateParamParser() {}

	static std::map<std::string, PhpPreg> regexs;
	static std::vector<std::string> regexs_ordered;
	const static int MAX_ITERATIONS;
	static PhpPreg COMMENT_REGEX;
	static PhpPreg MARKER_REGEX;
	static PhpPreg NOWIKI_REGEX;
	static PhpPreg BR_REGEX;

protected:
	static bool _getTemplates(std::string *data, std::map<std::string, std::string> *markers, std::vector<std::string> *templates, int start, int length);
};

} /* namespace phppreg */

#endif /* MWTEMPLATEPARAMPARSER_H_ */
