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

#ifndef PHPPREG_H_
#define PHPPREG_H_

#include <string>
#include <vector>
#include <memory>
#include <pcre.h>

#include "PregMatch.h"

#define OVECCOUNT (3 * 256)  /* should be a multiple of 3 */

namespace phppreg {

class PCRECPP_EXP_DEFN PhpPreg
{
public:
	/**
	 * Pattern compilation options
	 */
	enum COMPILE_OPTIONS {
		PREG_STUDY_PATTERN = 1,//!< PREG_STUDY_PATTERN Take extra time to study the pattern to improve performance
		PREG_USE_JIT = 2       //!< PREG_USE_JIT Use the jit compiler if available to improve performance. implies PREG_STUDY_PATTERN
	};

	/**
	 * constructor
	 *
	 * @param pattern Pattern to match against
	 * @param flags COMPILE_OPTIONS
	 */
	PhpPreg(const std::string& pattern, int flags = PREG_USE_JIT) { init(pattern, flags); }

	/**
	 * constructor
	 *
	 * @param pattern Pattern to match against
	 * @param flags COMPILE_OPTIONS
	 */
	PhpPreg(const char* pattern, int flags = PREG_USE_JIT) { init(pattern, flags); }

	/**
	 * constructor
	 *
	 * @param pattern Pattern to match against
	 * @param flags COMPILE_OPTIONS
	 */
	PhpPreg(const unsigned char* pattern, int flags = PREG_USE_JIT) { init(reinterpret_cast<const char*>(pattern), flags); }

	PhpPreg(const PhpPreg& other);

	PhpPreg() {}

	/**
	 * isError
	 *
	 * Check for an error after PhpPreg construction and match/matchAll
	 *
	 * @return true = error occurred, call getErrorMsg() to get message; false = no error
	 */
	inline bool isError() const { return (errmsg.length() != 0); }

	/**
	 * getErrorMsg
	 *
	 * Get an error message if an error occurred
	 *
	 * @return Error message
	 */
	std::string getErrorMsg() const { return errmsg; }

	/**
	 * match
	 *
	 * Search text for a pattern match, optionally return matched text.
	 *
	 * @param subject Text to perform matching on
	 * @param matches Vector to hold matches. NULL = don't return matches
	 * @param flags None
	 * @param offset Offset in bytes in subject to start matching at.
	 * @return 0 = no match or error, call isError() to determine if error; 1 = matched
	 */
	int match(const std::string& subject, MatchVector *matches = NULL, int flags = 0, int offset = 0);

	/**
	 * matchAll
	 *
	 * Search text for a pattern match repeatedly, optionally return matched text.
	 *
	 * @param subject Text to perform matching on
	 * @param matches Vector of MatchVectors to hold matches. NULL = don't return matches
	 * @param flags None
	 * @param offset Offset in bytes in subject to start matching at.
	 * @return 0 = no matches or error, call isError() to determine if error; >0 = match count
	 */
	int matchAll(const std::string& subject, std::vector<std::shared_ptr<MatchVector>> *matches = NULL, int flags = 0, int offset = 0);

	virtual ~PhpPreg();

protected:
	std::string errmsg;
	std::shared_ptr<pcre> re;
	std::shared_ptr<pcre_extra> study;
	std::map<std::string, int> nameMap;

	void init(const std::string& pattern, int flags);
	int matchImpl(const std::string& subject, void *matches, int flags, int offset, int matchall);
	void loadMatchVector(MatchVector& matches, int capcount, const std::string& subject, const int ovector[]) const;

private:
	PhpPreg(PhpPreg&& other) = delete;
	PhpPreg& operator= (const PhpPreg& other) = delete;
	PhpPreg& operator= (PhpPreg&& other) = delete;
};

} /* namespace phppreg */

#endif /* PHPPREG_H_ */
