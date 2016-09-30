/*
 * MWDumpHandler.h
 *
 *  Created on: Feb 4, 2016
 *      Author: bruce
 */

#ifndef MWDUMPHANDLER_H_
#define MWDUMPHANDLER_H_

#include <string>
#include <expat.h>

namespace phppreg {

class IPageHandler
{
public:
	virtual void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data) = 0;
	virtual ~IPageHandler() {}
};

class MWDumpHandler
{
public:
	MWDumpHandler(IPageHandler& pageHandler) : pageHandler(pageHandler) {}
	void startElement(void *userData, const char *el, const char **attr);
	void endElement(void *userData, const char *el);
	void characters(void *userData, const XML_Char *s, int len);
	virtual ~MWDumpHandler() {};

protected:
	std::string mwnamespace;
	std::string page_id;
	std::string revision_id;
	std::string page_data;
	IPageHandler& pageHandler;
	std::string container;
	std::string element;
	bool isRedirect = false;

private:
	MWDumpHandler() = delete;
};

} /* namespace phppreg */

#endif /* MWDUMPHANDLER_H_ */
