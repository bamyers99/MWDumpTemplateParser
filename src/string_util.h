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

#ifndef STRING_UTIL_H_
#define STRING_UTIL_H_

#include <string>
#include <vector>

/**
 * Replace all occurences of search string with replace string.
 *
 * @param subject String to do replacement in
 * @param search Search string
 * @param replace Replacement string
 * @param count Number of replacements to do. default (-1) = replace all occurences of search
 * @return number of replacements
 */
void string_replace(std::string *subject, const std::string& search, const std::string& replace, int count = -1);

/**
 * Trim whitespace from both ends of string
 * @param subject Search string
 * @param whitespace Whitespace characters
 */
void string_trim(std::string *subject, const std::string& whitespace = " \r\n\t");

void string_split(const std::string& subject, const std::string& separator, std::vector<std::string> *pieces, int maxcount = -1);

#endif /* STRING_UTIL_H_ */
