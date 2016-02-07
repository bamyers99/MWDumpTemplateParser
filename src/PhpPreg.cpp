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

#include "PhpPreg.h"

#include <map>
#include <iostream>
#include <sstream>

using namespace std;

namespace phppreg {

static const map<char, int> modifiers {
	{'i', PCRE_CASELESS},
	{'m', PCRE_MULTILINE},
	{'s', PCRE_DOTALL},
	{'x', PCRE_EXTENDED},
	{'A', PCRE_ANCHORED},
	{'D', PCRE_DOLLAR_ENDONLY},
	{'S', -1},
	{'U', PCRE_UNGREEDY},
	{'X', PCRE_EXTRA},
	{'J', PCRE_INFO_JCHANGED},
	{'u', PCRE_UTF8 | PCRE_UCP}
};

PhpPreg::PhpPreg(const PhpPreg& other)
{
	errmsg = other.errmsg;
	re = other.re;
	study = other.study;
	nameMap = other.nameMap;
}

PhpPreg::~PhpPreg()
{
	if (re != NULL) pcre_free(re);
	if (study != NULL) pcre_free_study(study);
}

/**
 * init
 */
void PhpPreg::init(const string& pattern, int flags)
{
	int options = 0;
	const char *errptr;
	int erroffset;

	if (pattern.length() < 3) {
		errmsg = "pattern too short - 3 char min";
		return;
	}

	// Get the delimiters
	char startDelim = pattern[0];
	char endDelim = startDelim;

	switch (startDelim) {
		case '(' :
		case ')' :
			endDelim = ')';
			break;
		case '{' :
		case '}' :
			endDelim = '}';
			break;
		case '[' :
		case ']' :
			endDelim = ']';
			break;
		case '<' :
		case '>' :
			endDelim = '>';
			break;
	}

	unsigned int endPos = pattern.rfind(endDelim);
	if (endPos == string::npos) {
		errmsg.append("pattern ending delimiter not found = ").append(1, endDelim);
		return;
	}

	// Get any pattern modifiers
	string mods = pattern.substr(endPos + 1);

	map<char,int>::const_iterator it;

	for (char c : mods) {
		if (c == ' ' || c == '\n') continue;
		it = modifiers.find(c);
		if (it == modifiers.end()){
			errmsg.append("invalid modifier = ").append(1, c);
			return;
		}

		if (it->second > 0) options |= it->second;
	}

	string realpattern = pattern.substr(1, endPos - 1);

	// Compile the pattern
	re = pcre_compile(realpattern.c_str(), options, &errptr, &erroffset, NULL);

	if (re == NULL) {
		ostringstream os;
		os << "compile error (" << errptr << ") at offset " << erroffset;
		errmsg = os.str();
		return ;
	}

	// Study the pattern
	if (flags & PREG_USE_JIT) flags |= PREG_STUDY_PATTERN;

	if (flags & PREG_STUDY_PATTERN) {
		int studyoptions = 0;
		if (flags & PREG_USE_JIT) studyoptions |= PCRE_STUDY_JIT_COMPILE;

		study = pcre_study(re, studyoptions, &errptr);

		if (study == NULL) {
			ostringstream os;
			os << "study error (" << errptr << ")";
			errmsg = os.str();
			return ;
		}
	}

	// Store named parameter offsets
	int namecount;

	pcre_fullinfo(re, study, PCRE_INFO_NAMECOUNT, &namecount);

	if (namecount > 0) {
		int name_entry_size;
		unsigned char *name_table;
		int n;

		// Get the name table address and name entry size

		pcre_fullinfo(re, study, PCRE_INFO_NAMETABLE, &name_table);
		pcre_fullinfo(re, study, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);

		// Store the offsets in the name map

		for (int i = 0; i < namecount; ++i)
		{
			n = (name_table[0] << 8) | name_table[1];
			nameMap.emplace(string(reinterpret_cast<const char*>(name_table + 2)), n);
			name_table += name_entry_size;
		}
	}
}

/**
 * match
 */
int PhpPreg::match(const string& subject, MatchVector *matches, int flags, int offset)
{
	return matchImpl(subject, matches, flags, offset, 0);
}

/**
 * matchAll
 */
int PhpPreg::matchAll(const string& subject, vector<shared_ptr<MatchVector>> *matches, int flags, int offset)
{
	return matchImpl(subject, matches, flags, offset, 1);
}

/**
 * matchImpl
 */
int PhpPreg::matchImpl(const string& subject, void *matches, int flags, int offset, int matchall)
{
	int ovector[OVECCOUNT];
	int rc;
	errmsg = "";

	if (matches) {
		if (matchall) ((vector<shared_ptr<MatchVector>> *)matches)->clear();
		else {
			((MatchVector *)matches)->clear();
			((MatchVector *)matches)->clearMap();
		}
	}

	rc = pcre_exec(re, study, subject.c_str(), subject.length(), offset, 0, ovector, OVECCOUNT);

	if (rc < 0) {
		switch (rc) {
			case PCRE_ERROR_NOMATCH:
				// Not an error
				break;

			case PCRE_ERROR_BADUTF8:
			case PCRE_ERROR_SHORTUTF8: {
				ostringstream os;
				os << "UTF8 error at offset " << ovector[0];
				errmsg = os.str();
				}
				break;

			default: {
				ostringstream os;
				os << "match error = " << rc;
				errmsg = os.str();
				}
				break;
		}

		return 0;
	}

	// Check for too many captures, and use max allowed captures
	if (rc == 0) {
		rc = OVECCOUNT/3;
	}

	if (! matchall) {
		if (matches) loadMatchVector(*(MatchVector *)matches, rc, subject, ovector);
		return 1;
	}

	if (matches) {
		MatchVector *mv = new MatchVector();
		loadMatchVector(*mv, rc, subject, ovector);
		((vector<shared_ptr<MatchVector>> *)matches)->emplace_back(mv);
	}

	int matchcount = 1;

	/* Before running the loop, check for UTF-8 and whether CRLF is a valid newline
	sequence. First, find the options with which the regex was compiled; extract
	the UTF-8 state, and mask off all but the newline options. */

	unsigned int option_bits;
	int subject_length = subject.length();

	pcre_fullinfo(re, study, PCRE_INFO_OPTIONS, &option_bits);
	int utf8 = option_bits & PCRE_UTF8;
	option_bits &= PCRE_NEWLINE_CR | PCRE_NEWLINE_LF | PCRE_NEWLINE_CRLF |
	               PCRE_NEWLINE_ANY |PCRE_NEWLINE_ANYCRLF;

	/* If no newline options were set, find the default newline convention from the
	build configuration. */

	if (option_bits == 0)
	  {
	  int d;
	  pcre_config(PCRE_CONFIG_NEWLINE, &d);
	  option_bits = (d == 13)? PCRE_NEWLINE_CR :
	          (d == 10)? PCRE_NEWLINE_LF :
	          (d == (13<<8 | 10))? PCRE_NEWLINE_CRLF :
	          (d == -2)? PCRE_NEWLINE_ANYCRLF :
	          (d == -1)? PCRE_NEWLINE_ANY : 0;
	  }

	// See if CRLF is a valid newline sequence.

	int crlf_is_newline =
	     option_bits == PCRE_NEWLINE_ANY ||
	     option_bits == PCRE_NEWLINE_CRLF ||
	     option_bits == PCRE_NEWLINE_ANYCRLF;

	// Loop for second and subsequent matches
	int options;
	int start_offset;

	for (;;) {
		options = 0;                 /* Normally no options */
		start_offset = ovector[1];   /* Start at end of previous match */

		/* If the previous match was for an empty string, we are finished if we are
		at the end of the subject. Otherwise, arrange to run another match at the
		same point to see if a non-empty match can be found. */

		if (ovector[0] == ovector[1]) {
			if (ovector[0] == subject_length) break;
			options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
	    }

		// Run the next matching operation
		rc = pcre_exec(re, study, subject.c_str(), subject.length(), start_offset, options, ovector, OVECCOUNT);

		/* This time, a result of NOMATCH isn't an error. If the value in "options"
		is zero, it just means we have found all possible matches, so the loop ends.
		Otherwise, it means we have failed to find a non-empty-string match at a
		point where there was a previous empty-string match. In this case, we do what
		Perl does: advance the matching position by one character, and continue. We
		do this by setting the "end of previous match" offset, because that is picked
		up at the top of the loop as the point at which to start again.

		There are two complications: (a) When CRLF is a valid newline sequence, and
		the current position is just before it, advance by an extra byte. (b)
		Otherwise we must ensure that we skip an entire UTF-8 character if we are in
		UTF-8 mode. */

		if (rc == PCRE_ERROR_NOMATCH) {
			if (options == 0) break;                    // All matches found
			ovector[1] = start_offset + 1;              // Advance one byte
			if (crlf_is_newline &&                      // If CRLF is newline &
					start_offset < subject_length - 1 &&    // we are at CRLF,
					subject[start_offset] == '\r' &&
					subject[start_offset + 1] == '\n')
				ovector[1] += 1;                          // Advance by one more.
			else if (utf8)                              // Otherwise, ensure we
			{                                         // advance a whole UTF-8
				while (ovector[1] < subject_length)       // character.
				{
					if ((subject[ovector[1]] & 0xc0) != 0x80) break;
					ovector[1] += 1;
				}
			}
			continue;    // Go round the loop again
		}

		// Other matching errors are not recoverable.

		if (rc < 0) {
			switch (rc) {
				case PCRE_ERROR_BADUTF8:
				case PCRE_ERROR_SHORTUTF8: {
					ostringstream os;
					os << "UTF8 error at offset " << ovector[0];
					errmsg = os.str();
					}
					break;

				default: {
					ostringstream os;
					os << "match error = " << rc;
					errmsg = os.str();
					}
					break;
			}

			return 0;
		}

		++matchcount;

		// Check for too many captures, and use max allowed captures
		if (rc == 0) {
			rc = OVECCOUNT/3;
		}

		if (matches) {
			MatchVector *mv = new MatchVector();
			loadMatchVector(*mv, rc, subject, ovector);
			((vector<shared_ptr<MatchVector>> *)matches)->emplace_back(mv);
		}
	} // End of loop to find second and subsequent matches

	return matchcount;
}

/**
 * loadMatchVector
 */
void PhpPreg::loadMatchVector(MatchVector& matches, int capcount, const string& subject, const int ovector[]) const
{
	const char *subject_ptr = subject.c_str();
	int startPos;

	if (nameMap.size()) matches.fillMap(nameMap);

	for (int i = 0; i < capcount; ++i) {
		startPos = ovector[2*i];
		matches.addItem(startPos, subject_ptr + startPos, ovector[2*i+1] - startPos);
	}
}


} /* namespace phppreg */
