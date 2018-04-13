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


#ifndef CASERVICE_H_
#define CASERVICE_H_

/**
 * @addtogroup cam
 * @{
 */

#include <boost/thread.hpp>
#include <common/config/config.h>
#include <common/utility/CommunicationReceiver.h>
#include <common/utility/CommunicationSender.h>
#include <common/utility/Constants.h>
#include <common/buffers/data.pb.h>
#include <common/buffers/cam.pb.h>
#include <common/buffers/gps.pb.h>
#include <common/buffers/obd2.pb.h>
#include <common/buffers/camInfo.pb.h>
#include <common/buffers/CoopAwareness.pb.h>
#include <common/buffers/ItsPduHeader.pb.h>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <mutex>
#include <common/asn1/CAM.h>
#include <common/messages/MessageUtils.h>

/** Struct that hold the configuration for CaService.
 * The configuration is defined in <a href="../../cam/config/config.xml">cam/config/config.xml</a>
 */
struct CaServiceConfig {
	bool mGenerateMsgs;
	int mExpirationTime;
	int mMaxGpsAge;
	int mMaxObd2Age;
	double mThresholdRadiusForHeading;
	bool mIsRSU;

	void loadConfig(ptree& pt) {
		
		
		
		mGenerateMsgs = pt.get("cam.generateMsgs", true);
		mExpirationTime = pt.get("cam.expirationTime", 1);
		mMaxGpsAge = pt.get("cam.maxGpsAge", 10);
		mMaxObd2Age = pt.get("cam.maxObd2Age", 10);
		mThresholdRadiusForHeading = pt.get("cam.thresholdRadiusForHeading", 0.3);
		mIsRSU = pt.get("cam.isRSU", false);

		
	}
};

/**
 * Class that handles the receiving, creating and sending of CA Messages.
 *
 * @nonStandard Outdated CAMs queued in ath9k hardware can not be flushed. Ath9k driver developer, Adrian Chadd says:
 * <div>
 * "Once they're in the hardware queue then they're there. You'd have to
 * stop the TX queue (which if it fails, means you have to reset the
 * chip) before you can fiddle with the TX queue. But with TX FIFO chips
 * (AR9380 and later), you're pushing TX entries into the FIFO, and you
 * can't remove them. AND, the pre-FIFO chips (AR928x and earlier) the
 * MAC doesn't always re-read TXDP when you start TX - it only is
 * guaranteed to read it once, after a reset, and after that you
 * shouldn't overwrite it or it occasionally ignores you."
 * </div>
 */
class CaService {
public:
	CaService(CaServiceConfig &config, ptree& configTree);
	~CaService();

	/** Sends a new CAM to LDM and DCC.	 */
	void send();

	/** Calculates the heading towards North based on the two specified coordinates.
	 *
	 * @param lat1 Latitude of coordinate 1.
	 * @param lon1 Longitude of coordinate 1.
	 * @param lat2 Latitude of coordinate 2.
	 * @param lon2 Longitude of coordinate 2.
	 * @return The heading in degrees.
	 */
	double getHeading(double lat1, double lon1, double lat2, double lon2);

	/** Calculates the distance between the two specified coordinates
	 *
	 * @param lat1 Latitude of coordinate 1.
	 * @param lon1 Longitude of coordinate 1.
	 * @param lat2 Latitude of coordinate 2.
	 * @param lon2 Longitude of coordinate 2.
	 * @return The distance in meters.
	 */
	double getDistance(double lat1, double lon1, double lat2, double lon2);

private:
	/** Receives incoming CAMs from DCC and forwards them to LDM.
	 * Receives serialized DATA packages from DCC, deserializes it, and forwards the contained serialized CAM to LDM.
	 */
	void receive();

	/** Sends information about why a CAM was triggered to LDM.
	 * Sends serialized camInfo to LDM, including the current time and the specified reason for triggering.
	 * @param triggerReason Difference in time, heading, position, or speed.
	 * @param delta Specifies how big the difference in time, heading, position, or speed is.
	 */
	void sendCamInfo(std::string triggerReason, double delta);

	/** Periodically checks the rules for triggering a new CAM.
	 * Checks every 100ms whether the difference in time, heading, position, or speed requires the triggering of a new CAM. Reasons for triggering are:
	 * The time difference since the last triggered CAM is at least one second.
	 * The heading since the last CAM changed by more than four degrees.
	 * The position since the last CAM changed by more than five meters.
	 * The speed since the last CAM changed by more than 1m/s.
	 * @param ec Boost error code
	 */
	void alarm(const boost::system::error_code &ec);

	/** Triggers sending of CAM.
	 *
	 */
	void trigger();

	/** Generates a new unaligned PER compliant CAM.
	 * The new CAM includes the MAC address as stationId, an increasing but not unique ID, a current time stamp, and the latest GPS and OBD2 data if it is not too old (as configured).
	 * @return The newly generated CAM.
	 */
	/*std::vector<uint8_t>*/CAM_t* generateCam();

	/** Converts ASN1 CAM structure into CAM protocol buffer.
	 * @return The newly generated CAM protocol buffer.
	 */
	camPackage::CAM convertAsn1toProtoBuf(CAM_t* cam);

	/** Receives new GPS data from the GPS module.
	 *
	 */
	void receiveGpsData();

	/** Receives new OBD2 data from the OBD2 module.
	 *
	 */
	void receiveObd2Data();

	/** Checks if heading has changed more than 4 degrees.
	 * @return True if CAM needs to be triggered, false otherwise
	 */
	bool isHeadingChanged();

	/** Checks if position has changed more than 5 metres.
	 * @return True if CAM needs to be triggered, false otherwise
	 */
	bool isPositionChanged();

	/** Checks if speed has changed more than 1 m/sec.
	 * @return True if CAM needs to be triggered, false otherwise
	 */
	bool isSpeedChanged();

	/** Checks if time more than 1 second has past since last CAM.
	 * @return True if CAM needs to be triggered, false otherwise
	 */
	bool isTimeToTriggerCAM();

	/** Schedules next triggering checks for new CAM.
	 *
	 */
	void scheduleNextAlarm();

	/** Checks if the last received GPS data is still valid.
	 * @return True if GPS data is valid, false otherwise.
	 */
	bool isGPSdataValid();

	GlobalConfig mGlobalConfig;
	CaServiceConfig mConfig;

	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverFromDcc;
	CommunicationReceiver* mReceiverGps;
	CommunicationReceiver* mReceiverObd2;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadObd2DataReceive;

	MessageUtils* mMsgUtils;
	LoggingUtility* mLogger;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	long mIdCounter;
	double mCamTriggerInterval;

	gpsPackage::GPS mLatestGps;
	/**
	 * True iff a GPS was received and it is not too old.
	 */
	bool mGpsValid;		//true if GPS was received and it's not too old
	std::mutex mMutexLatestGps;

	obd2Package::OBD2 mLatestObd2;
	/**
	 * True iff a OBD2 was received and it is not too old.
	 */
	bool mObd2Valid;
	std::mutex mMutexLatestObd2;

	struct LastSentCamInfo {
		bool hasGPS = false;
		gpsPackage::GPS lastGps;
		bool hasOBD2 = false;
		obd2Package::OBD2 lastObd2;
		double lastHeading;
		int64_t timestamp = 0;
	};
	LastSentCamInfo mLastSentCamInfo;
};

/** @} */ //end group
#endif
