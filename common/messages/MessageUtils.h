// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


#ifndef MESSAGE_UTILS_H_
#define MESSAGE_UTILS_H_

#include <common/utility/LoggingUtility.h>
#include <common/asn1/asn_application.h>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>


/**
 * Contains message utility functions (encoding/ decoding) used by other modules.
 *
 * @ingroup common
 */
class MessageUtils {
public:
	MessageUtils(LoggingUtility& logger);

	static int writeOut(const void *buffer, size_t size, void *app_key);
	std::vector<uint8_t> encodeMessage(asn_TYPE_descriptor_t *td, void *structPtr);
	int decodeMessage(asn_TYPE_descriptor_t* td, void** t, std::string buffer);

	LoggingUtility& mLogger;
};

#endif /* UTILS_H_ */
