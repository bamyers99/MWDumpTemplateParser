/*
 * MWDumpHandler.cpp
 *
 *  Created on: Feb 4, 2016
 *      Author: bruce
 */

#include "MWDumpHandler.h"

namespace phppreg {

void MWDumpHandler::startElement(void *userData, const char *el, const char **attr)
{
	element = el;

	if (element == "page") {
		if (container.length() == 0) container = "page";
	}

	else if (element == "revision") {
		if (container == "page") container = "revision";
	}

	else if (element == "redirect") {
		if (container == "page") isRedirect = true;
	}
}

void MWDumpHandler::endElement(void *userData, const char *el)
{
	if (container == "page") {
		element = el;
		if (element == "page") {
			int mwn = atoi(mwnamespace.c_str());
			unsigned int pid = atoi(page_id.c_str());
			unsigned int rid = atoi(revision_id.c_str());
			if (! isRedirect) pageHandler.processPage(mwn, pid, rid, page_data, page_title);
			container.clear(); element.clear(); page_id.clear(); mwnamespace.clear(); page_data.clear(); revision_id.clear(); isRedirect = false;
			page_title.clear();
		}
	}

	else if (container == "revision") {
		element = el;
		if (element == "revision") container = "page";
	}

	element.clear();
}

void MWDumpHandler::characters(void *userData, const XML_Char *s, int len)
{
	if (container == "page") {
		if (element == "ns") mwnamespace.append((char *)s, len);
		else if (element == "id") page_id.append((char *)s, len);
		else if (element == "title") page_title.append((char *)s, len);
	}

	else if (container == "revision") {
		if (element == "id") revision_id.append((char *)s, len);
		else if (element == "text") page_data.append((char *)s, len);
	}
}

} /* namespace phppreg */
