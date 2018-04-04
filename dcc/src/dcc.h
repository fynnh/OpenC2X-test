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
// Florian Klingler <klingler@ccs-labs.org>


#ifndef DCC_H_
#define DCC_H_

/**
 * @addtogroup dcc
 * @{
 */

#include "DccConfig.h"
#include "State.h"
#include "RingBuffer.h"
#include "LeakyBucket.h"
#include <mutex>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <common/utility/CommunicationReceiver.h>
#include <common/utility/CommunicationSender.h>
#include <common/utility/LoggingUtility.h>
#include <common/buffers/data.pb.h>
#include <common/buffers/dccInfo.pb.h>
#include "SendToHardwareViaMAC.h"
#include "ReceiveFromHardwareViaMAC.h"
#include "ChannelProber.h"
#include "PktStatsCollector.h"
#include <common/messages/MessageUtils.h>
#include <random>

/**
 * Decentralized Congestion Control.
 */
class DCC {
public:
	DCC(bool setUpWlan);
	~DCC();


	/** Initializes all modules, threads, and timers.
	 * The channel prober is started if the channel load is not simulated.
	 * Threads for receiving CAMs and DENMs are started.
	 * The timers for measuring the channel load, packet stats, for updating the DCC states, and for collecting and sending DccInfo to LDM are started.
	 */
	void init();

	/** Initializes leaky buckets.
	 *
	 */
	void initLeakyBuckets();

	/** Initializes states and sets default values.
	 *
	 * @param numActiveStates Number of active states. Currently only one active state is supported/tested.
	 */
	void initStates(int numActiveStates);


	/** Receives CAMs from CaService.
	 * Continuously receives CAMs generated by CaService and enqueues them in the queue of the correct access category (BE).
	 * Before enqueuing, outdated CAMs are flushed from the queue.
	 * If a CAM is enqueued successfully, sendQueuedPackets() is called. Else, the queue is full and the CAM is dropped.
	 */
	void receiveFromCa();

	/** Receives standard compliant CAMs from CaService.
	 * Continuously receives CAMs generated by CaService and enqueues them in the queue of the correct access category (BE).
	 * Before enqueuing, outdated CAMs are flushed from the queue.
	 * If a CAM is enqueued successfully, sendQueuedPackets() is called. Else, the queue is full and the CAM is dropped.
	 */
	void receiveFromCa2();

	/** Receives DENMs from CaService.
	 * Continuously receives DENMs generated by CaService and enqueues them in the queue of the correct access category (VI).
	 * Before enqueuing, outdated DENMs are flushed from the queue.
	 * If a DENM is enqueued successfully, sendQueuedPackets() is called. Else, the queue is full and the DENM is dropped.
	 */
	void receiveFromDen();

	/** Receives CAMs or DENMs from hardware and forwards them to the corresponding module (CaService or DenService).
	 * The messages are contained in a serialized data protobuf packet.
	 */
	void receiveFromHw();

	/** Receives standard compliant CAMs or DENMs from hardware and forwards them to the corresponding module (CaService or DenService).
	 * The messages are contained in a serialized data protobuf packet.
	 */
	void receiveFromHw2();


	/** Periodically sends network info in form of four protobuf packets (one for each AC).
	 * Includes the current time, channel load, state, available tokens, queued packets, DCC-mechanism, TX power,
	 * token interval, data rate, and carrier sense for each access category. The access category itself is also contained.
	 * @param ec Boost error code
	 */
	void sendDccInfo(const boost::system::error_code& ec);

	/** Sets the current state and updates corresponding characteristics and intervals.
	 *
	 * @param state The new state.
	 */
	void setCurrentState(int state);

	/** Simulates realistic channel load for testing.
	 *
	 * @return Average channel load over 10000 probings (corresponding to a probing interval of 10 µs).
	 */
	double simulateChannelLoad();

	/** Periodically gets the measured (or simulated) channel load and stores it in 2 ring buffers (to decide state changes).
	 *
	 * @param ec Boost error code
	 */
	void measureChannel(const boost::system::error_code& ec);

	/** Periodically gets and sets the packet stats (about flushed packets).
	 *
	 * @param ec Boost error code
	 */
	void measurePktStats(const boost::system::error_code& ec);

	/** Periodically updates state according to recent channel load measurements.
	 * Currently only the states relaxed, active1, and restricted are supported.
	 * @param ec Boost error code
	 */
	void updateState(const boost::system::error_code& ec);

	/** Periodically adds a token=send-permit to the specified bucket of an AC.
	 *
	 * @param ec Boost error code
	 * @param ac Access category
	 */
	void addToken(const boost::system::error_code& ec, Channels::t_access_category ac);

	/** Reschedules timer for next "addToken" when switching to another state.
	 * Reschedule based on last token that was added rather than time of state-switch.
	 * @param ac Access category
	 */
	void rescheduleAddToken(Channels::t_access_category ac);

	/** Sends queued packets from specified leaky bucket to hardware until out of packets or tokens.
	 *
	 * @param ac Access category
	 */
	void sendQueuedPackets(Channels::t_access_category ac);

	/** Sets txPower and data rate for a packet (before being sent).
	 *
	 * @param data Packet to be sent.
	 */
	void setMessageLimits(dataPackage::DATA* data);


	/** Getter for the current DCC-mechanism.
	 *
	 * @param ac Access category
	 * @return DCC-mechanism
	 */
	dcc_Mechanism_t currentDcc(Channels::t_access_category ac);

	/** Getter for the current TX power.
	 *
	 * @param ac Access category
	 * @return TX power
	 */
	double currentTxPower(Channels::t_access_category ac);

	/** Getter for the current token interval.
	 *
	 * @param ac Access category
	 * @return token interval
	 */
	double currentTokenInterval(Channels::t_access_category ac);

	/** Getter for the current data rate.
	 *
	 * @param ac Access category
	 * @return data rate
	 */
	double currentDatarate(Channels::t_access_category ac);

	/** Getter for the current carrier sense.
	 *
	 * @param ac Access category
	 * @return carrier sense
	 */
	double currentCarrierSense(Channels::t_access_category ac);

private:
	/**
	 * All used access categories (ACs): VI, VO, BE, BK.
	 */
	const Channels::t_access_category mAccessCategories[4] = {Channels::AC_VI, Channels::AC_VO, Channels::AC_BE, Channels::AC_BK};	//all used ACs

	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverFromDen;
	CommunicationSender* mSenderToServices;
	CommunicationSender* mSenderToLdm;

	LoggingUtility* mLogger;
	LoggingUtility* channelProberLogger;
	LoggingUtility* pktStatsCollectorLogger;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveFromHw;

	SendToHardwareViaMAC* mSenderToHw;
	ReceiveFromHardwareViaMAC* mReceiverFromHw;

	boost::asio::io_service mIoService;
	boost::asio::io_service::strand mStrand;
	boost::asio::deadline_timer* mTimerMeasureChannel;
	boost::asio::deadline_timer* mTimerMeasurePktStats;
	boost::asio::deadline_timer* mTimerStateUpdate;
	boost::asio::deadline_timer* mTimerDccInfo;

	MessageUtils* mMsgUtils;

	/**
	 * Timers to periodically add tokens for all four ACs.
	 */
	std::map<Channels::t_access_category, boost::asio::deadline_timer*> mTimerAddToken;	//timers for all four ACs

	std::default_random_engine mRandNumberGen;
	std::bernoulli_distribution mBernoulli;
	std::uniform_real_distribution<double> mUniform;

	/**
	 * Holds the recent channel load measurements (influences state changes).
	 */
	RingBuffer<double> mChannelLoadInTimeUp;	//holds the recent channel load measurements (influences state changes)
	RingBuffer<double> mChannelLoadInTimeDown;

	/**
	 * Leaky buckets for all four ACs to hold the available tokens and enqueued packets.
	 */
	std::map<Channels::t_access_category, LeakyBucket<dataPackage::DATA>*> mBucket;	//LeakyBuckets for all four ACs

	GlobalConfig mGlobalConfig;
	DccConfig mConfig;

	States states;			//map of all states
	int mCurrentStateId;
	State* mCurrentState;

	ChannelProber* mChannelProber;
	double mChannelLoad;

	PktStatsCollector* mPktStatsCollector;
	PktStats mPktStats;
	int mStatIdx;
	uint32_t prev_be_flush_req;
	uint32_t prev_be_flush_not_req;
	
	std::mutex mMutexLastTokenAt;
	std::map<Channels::t_access_category, bool> mAddedFirstToken;					//was any token added in this state, yet?
	std::map<Channels::t_access_category, boost::posix_time::ptime> mLastTokenAt;	//when was the last token added in this state
};

/**
 * @}
 */
#endif
