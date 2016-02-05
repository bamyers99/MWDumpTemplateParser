/*
 * MWDumpHandler.h
 *
 *  Created on: Feb 4, 2016
 *      Author: bruce
 */

#ifndef MWDUMPHANDLER_H_
#define MWDUMPHANDLER_H_

#include <xercesc/sax2/DefaultHandler.hpp>
#include <string>

namespace phppreg {

class IPageHandler
{
public:
	virtual void processPage(int mwnamespace, unsigned int page_id, unsigned int revision_id, const std::string& page_data) = 0;
	virtual ~IPageHandler() {}
};

class MWDumpHandler : public xercesc::DefaultHandler
{
public:
	MWDumpHandler(IPageHandler& pageHandler) : pageHandler(pageHandler) {};
	void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const xercesc::Attributes& attrs);
	void endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname);
	void characters(const XMLCh* const chars, const unsigned int length);
	virtual ~MWDumpHandler() {};

protected:
	std::string mwnamespace;
	std::string page_id = 0;
	std::string revision_id = 0;
	std::string page_data;
	IPageHandler& pageHandler;
	std::string container;
	std::string element;
	char transbuf[2048];
	XMLTranscoder* utf8Transcoder;

private:
	MWDumpHandler() = delete;
};

} /* namespace phppreg */

#endif /* MWDUMPHANDLER_H_ */
