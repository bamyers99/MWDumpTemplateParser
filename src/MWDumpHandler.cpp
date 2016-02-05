/*
 * MWDumpHandler.cpp
 *
 *  Created on: Feb 4, 2016
 *      Author: bruce
 */

#include "MWDumpHandler.h"
#include <xercesc/util/XMLString.hpp>

using namespace xercesc;

namespace phppreg {

void MWDumpHandler::startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const xercesc::Attributes& attrs)
{
	element = XMLString::transcode(localname);

	if (element == "page") {
		if (container.length() == 0) container = "page";
	}

	else if (element == "revision") {
		if (container == "page") container = "revision";
	}
}

void MWDumpHandler::endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname)
{
	if (container == "page") {
		element = XMLString::transcode(localname);
		if (element == "page") {

			pageHandler.processPage(mwnamespace, page_id, revision_id, page_data);
			container.clear(); element.clear(); page_id.clear(); mwnamespace.clear(); page_data.clear(); revision_id.clear();
		}
	}

	else if (container == "revision") {
		element = XMLString::transcode(localname);
		if (element == "revision") container = "page";
	}

	element.clear();
}

void MWDumpHandler::characters(const XMLCh* const chars, const unsigned int length)
{
	if (container == "page") {
		if (element == "ns") mwnamespace .=
	}

	else if (container == "revision") {

	}

   	switch ($this->parserState['container']) {
    		case 'page':
    			switch ($this->parserState['element']) {
    				case 'ns':
    					$this->parserState['namespace'] .= $data;
    					break;

    				case 'id':
    					$this->parserState['page_id'] .= $data;
    					break;
    			}
    			break;

    		case 'revision':
    			switch ($this->parserState['element']) {
    				case 'id':
    					$this->parserState['revision_id'] .= $data;
    					break;

    				case 'text':
    					$this->parserState['data'] .= $data;
    					break;
    			}
    			break;
    	}

}

} /* namespace phppreg */
