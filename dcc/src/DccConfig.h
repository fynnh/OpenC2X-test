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


#ifndef DCCCONFIG_H_
#define DCCCONFIG_H_

/**
 * @addtogroup dcc
 * @{
 */
#include <map>
#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include <common/utility/Constants.h>
#include <common/config/config.h>

using boost::property_tree::ptree;


namespace {
	const int STATE_UNDEF = -1;
	const int STATE_RELAXED = 0;
	const int STATE_ACTIVE1 = 1;
	const int STATE_RESTRICTED = 2;		//TODO: has to be adjusted when using more than one active state
}

enum dcc_Mechanism_t {
	dcc_NONE = 0,
	dcc_TPC = 1,
	dcc_TRC = 2,
	dcc_TPC_TRC = 3,
	dcc_TDC = 4,
	dcc_TPC_TDC = 5,
	dcc_TPC_TRC_TDC = 7,
	dcc_DSC = 8,
	dcc_TAC = 16,
	dcc_ALL = 63
};

namespace Channels {
	enum t_channel_number {
		CRIT_SOL = 172,
		SCH1 = 174,
		SCH2 = 176,
		CCH = 178,
		SCH3 = 180,
		SCH4 = 182,
		HPPS = 184,
		NO_CHANNEL = -1
	};

	enum t_access_category {
		AC_BK = PRIORITY_BK,
		AC_BE = PRIORITY_BE,
		AC_VI = PRIORITY_VI,
		AC_VO = PRIORITY_VO,
		NO_AC = -1,
	};

	enum t_bitRate {
		bR_AUTO = -1,
		bR_3_Mbps = 3000000,
		bR_4p5_Mbps = 4500000,
		bR_6_Mbps = 6000000,
		bR_9_Mbps = 9000000,
		bR_12_Mbps = 12000000,
		bR_18_Mbps = 18000000,
		bR_24_Mbps = 24000000,
		bR_27_Mbps = 27000000
	};
}

const Channels::t_access_category mAccessCategories[4] = {Channels::AC_VI, Channels::AC_VO, Channels::AC_BE, Channels::AC_BK};	//all used ACs

/**
 * Template class to hold a NDL value from the config.
 */
template<typename T> class DCC_NDL_value {
	public:
		bool isRef;
		T val;
};

/**
 * Template class to hold a NDL parameter from the config.
 */
template<typename T> class DCC_NDL_Param {
	public:
		std::map<Channels::t_access_category, DCC_NDL_value<T> > ac;
		DCC_NDL_Param() {};
		DCC_NDL_Param(const DCC_NDL_Param<T>& param) {
			ac = param.ac;
		}
		~DCC_NDL_Param() {};
};

/**
 * Class that holds the config of a state such as the corresponding DCC-mechanism, TX power, token interval, data rate, carrier sense, etc.
 */
class DCC_State_Config {
	public:
		size_t asStateId;
		double asChanLoad;
		DCC_NDL_Param<dcc_Mechanism_t> asDcc;
		DCC_NDL_Param<double> asTxPower;
		DCC_NDL_Param<double> asPacketInterval;
		DCC_NDL_Param<double> asDatarate;
		DCC_NDL_Param<double> asCarrierSense;

		DCC_State_Config(size_t asStateId, double asChanLoad, dcc_Mechanism_t asDcc_def, double asTxPower_def, double asPacketInterval_def, double asDatarate_def, double asCarrierSense_def) {
			this->asStateId = asStateId;
			this->asChanLoad = asChanLoad;

			// asDcc
			DCC_NDL_value<dcc_Mechanism_t> asDcc_val;
			asDcc_val.isRef = false;
			asDcc_val.val = asDcc_def;

			// asTxPower
			DCC_NDL_value<double> asTxPower_val;
			asTxPower_val.isRef = false;
			asTxPower_val.val = asTxPower_def;

			// asPacketInterval TODO: rename to tokenInterval?
			DCC_NDL_value<double> asPacketInterval_val;
			asPacketInterval_val.isRef = false;
			asPacketInterval_val.val = asPacketInterval_def;

			// asDatarate
			DCC_NDL_value<double> asDatarate_val;
			asDatarate_val.isRef = false;
			asDatarate_val.val = asDatarate_def;

			// asCarrierSense
			DCC_NDL_value<double> asCarrierSense_val;
			asCarrierSense_val.isRef = false;
			asCarrierSense_val.val = asCarrierSense_def;

			//set for each AC
			for (Channels::t_access_category accessCategory : mAccessCategories) {
				asDcc.ac[accessCategory] = asDcc_val;
				asTxPower.ac[accessCategory] = asTxPower_val;
				asPacketInterval.ac[accessCategory] = asPacketInterval_val;
				asDatarate.ac[accessCategory] = asDatarate_val;
				asCarrierSense.ac[accessCategory] = asCarrierSense_val;
			}
		}

		~DCC_State_Config() {
		}
};

/**
 * Struct that holds the config of DCC such as the size of the leaky buckets and constants concerning TRC, TPC, TDC, and DSC.
 */
struct DccConfig {
	// General
	std::string xml_file;
	/**
	 * Specifies whether own sent messages should be ignored or received like messages from any other station (looped back).
	 */
	bool ignoreOwnMessages;
	/**
	 * Specifies whether the channel load should be simulated or measured (only works with ath9k).
	 */
	bool simulateChannelLoad;	//set to true to simulate channel load, false to use real data

	/**
	 * Interval in which dccInfo/network is sent to LDM (default: 1000ms = 1s).
	 */
	int dccInfoInterval;		//interval in which dccInfo/network info is sent to LDM (default: 1000ms = 1s)

	// Leaky Bucket
	int bucketSize_AC_VI;
	int bucketSize_AC_VO;
	int bucketSize_AC_BE;
	int bucketSize_AC_BK;

	int queueSize_AC_VI;
	int queueSize_AC_VO;
	int queueSize_AC_BE;
	int queueSize_AC_BK;

	// TRC
	double NDL_minPacketInterval;
	double NDL_refPacketInterval;
	double NDL_maxPacketInterval;
	double NDL_defPacketInterval;

	// TPC
	double NDL_maxTxPower;
	double NDL_minTxPower;
	double NDL_refTxPower;
	double NDL_defTxPower;

	// TDC
	double NDL_minDatarate;
	double NDL_maxDatarate;
	double NDL_refDatarate;
	double NDL_defDatarate;

	// DSC
	double NDL_minCarrierSense;
	double NDL_maxCarrierSense;
	double NDL_refCarrierSense;
	double NDL_defCarrierSense;

	// Channel Load
	double NDL_minChannelLoad;
	double NDL_maxChannelLoad;

	// DCC Process
	double NDL_timeUp;
	double NDL_timeDown;
	double DCC_measure_interval_Tm;
	double NDL_minDccSampling;

	// Stats related to flushing outdated packets in hardware queues
	double DCC_collect_pkt_flush_stats;


	//Configuration of STATEs
	size_t NDL_numActiveState;
	std::map<size_t, DCC_State_Config> stateConfig;

	void loadParameters(ptree& pt) {
		simulateChannelLoad = pt.get("dcc.simulateChannelLoad", true);
		ignoreOwnMessages = pt.get("dcc.ignoreOwnMessages", false);
		dccInfoInterval = pt.get("dcc.dccInfoInterval", 1000);

		load_NDL_Parameters(pt);
	}

	void load_NDL_Parameters(ptree pt) {
		bucketSize_AC_VI = pt.get("dcc.bucketSize_AC_VI", 1);
		bucketSize_AC_VO = pt.get("dcc.bucketSize_AC_VO", 1);
		bucketSize_AC_BE = pt.get("dcc.bucketSize_AC_BE", 1);
		bucketSize_AC_BK = pt.get("dcc.bucketSize_AC_BK", 1);

		queueSize_AC_VI = pt.get("dcc.queueSize_AC_VI", 0);
		queueSize_AC_VO = pt.get("dcc.queueSize_AC_VO", 0);
		queueSize_AC_BE = pt.get("dcc.queueSize_AC_BE", 0);
		queueSize_AC_BK = pt.get("dcc.queueSize_AC_BK", 0);

		NDL_minPacketInterval = pt.get("dcc.NDL_minPacketInterval", 0.04);
		NDL_refPacketInterval = pt.get("dcc.NDL_refPacketInterval", 0.50);
		NDL_maxPacketInterval = pt.get("dcc.NDL_maxPacketInterval", 1.00);
		NDL_defPacketInterval = pt.get("dcc.NDL_defPacketInterval", 0.50);

		NDL_maxTxPower = pt.get("dcc.NDL_maxTxPower", 33.00);
		NDL_minTxPower = pt.get("dcc.NDL_minTxPower", -10.00);
		NDL_defTxPower = pt.get("dcc.NDL_defTxPower", 23.00);

		NDL_minDatarate = pt.get("dcc.NDL_minDatarate", 3.00);
		NDL_maxDatarate = pt.get("dcc.NDL_maxDatarate", 12.00);
		NDL_defDatarate = pt.get("dcc.NDL_defDatarate", 6.00);

		NDL_minCarrierSense = pt.get("dcc.NDL_minCarrierSense", -95.0);
		NDL_maxCarrierSense = pt.get("dcc.NDL_maxCarrierSense", -65.0);
		NDL_defCarrierSense = pt.get("dcc.NDL_defCarrierSense", -85.0);

		NDL_minChannelLoad = pt.get("dcc.NDL_minChannelLoad", 0.15);
		NDL_maxChannelLoad = pt.get("dcc.NDL_maxChannelLoad", 0.40);

		NDL_timeUp = pt.get("dcc.NDL_timeUp", 1.00);
		NDL_timeDown = pt.get("dcc.NDL_timeDown", 5.00);
		DCC_measure_interval_Tm = pt.get("dcc.DCC_measure_interval_Tm", 1.00);

		DCC_collect_pkt_flush_stats = pt.get("dcc.DCC_collect_pkt_flush_stats", 1.00);

		NDL_minDccSampling = pt.get("dcc.NDL_minDccSampling", 1.00);


		NDL_numActiveState = pt.get("dcc.NDL_numActiveState", 1);

		// RELAXED State
		stateConfig.insert(std::make_pair(STATE_RELAXED, DCC_State_Config(STATE_RELAXED, NDL_minChannelLoad, dcc_ALL, NDL_maxTxPower, NDL_minPacketInterval, NDL_minDatarate, NDL_minCarrierSense)));

		// ACTIVE States
		for(size_t i = 1; i<=NDL_numActiveState; i++) {

			DCC_State_Config cfg = DCC_State_Config(i, NDL_minChannelLoad, dcc_ALL, NDL_maxTxPower, NDL_minPacketInterval, NDL_minDatarate, NDL_minCarrierSense);

			cfg.asChanLoad = pt.get("dcc.NDL_asChanLoad_active"+std::to_string(i), 0.20);


			//NDL_asDcc_active
			std::string NDL_asDcc_active_VI = pt.get<std::string>("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_VI", "0");
			std::string NDL_asDcc_active_VO = pt.get<std::string>("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_VO", "1");
			std::string NDL_asDcc_active_BE = pt.get<std::string>("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_BE", "1");
			std::string NDL_asDcc_active_BK = pt.get<std::string>("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_BK", "1");

			cfg.asDcc.ac[Channels::AC_VI].isRef = (NDL_asDcc_active_VI == "ref"?true:false);
			cfg.asDcc.ac[Channels::AC_VO].isRef = (NDL_asDcc_active_VO == "ref"?true:false);
			cfg.asDcc.ac[Channels::AC_BE].isRef = (NDL_asDcc_active_BE == "ref"?true:false);
			cfg.asDcc.ac[Channels::AC_BK].isRef = (NDL_asDcc_active_BK == "ref"?true:false);

			cfg.asDcc.ac[Channels::AC_VI].val = (dcc_Mechanism_t) pt.get("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_VI", 0);
			cfg.asDcc.ac[Channels::AC_VO].val = (dcc_Mechanism_t) pt.get("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_VO", 1);
			cfg.asDcc.ac[Channels::AC_BE].val = (dcc_Mechanism_t) pt.get("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_BE", 1);
			cfg.asDcc.ac[Channels::AC_BK].val = (dcc_Mechanism_t) pt.get("dcc.NDL_asDcc_active"+std::to_string(i)+"_AC_BK", 1);


			//NDL_asTxPower_active
			std::string NDL_asTxPower_active_VI = pt.get<std::string>("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_VI", "ref");
			std::string NDL_asTxPower_active_VO = pt.get<std::string>("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_VO", "25.00");
			std::string NDL_asTxPower_active_BE = pt.get<std::string>("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_BE", "20.00");
			std::string NDL_asTxPower_active_BK = pt.get<std::string>("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_BK", "15.00");

			cfg.asTxPower.ac[Channels::AC_VI].isRef = (NDL_asTxPower_active_VI == "ref"?true:false);
			cfg.asTxPower.ac[Channels::AC_VO].isRef = (NDL_asTxPower_active_VO == "ref"?true:false);
			cfg.asTxPower.ac[Channels::AC_BE].isRef = (NDL_asTxPower_active_BE == "ref"?true:false);
			cfg.asTxPower.ac[Channels::AC_BK].isRef = (NDL_asTxPower_active_BK == "ref"?true:false);

			cfg.asTxPower.ac[Channels::AC_VI].val = pt.get("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_VI", NDL_defTxPower);
			cfg.asTxPower.ac[Channels::AC_VO].val = pt.get("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_VO", 25.00);
			cfg.asTxPower.ac[Channels::AC_BE].val = pt.get("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_BE", 20.00);
			cfg.asTxPower.ac[Channels::AC_BK].val = pt.get("dcc.NDL_asTxPower_active"+std::to_string(i)+"_AC_BK", 15.00);


			//NDL_asPacketInterval_active
			std::string NDL_asPacketInterval_active_VI = pt.get<std::string>("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_VI", "ref");
			std::string NDL_asPacketInterval_active_VO = pt.get<std::string>("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_VO", "ref");
			std::string NDL_asPacketInterval_active_BE = pt.get<std::string>("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_BE", "ref");
			std::string NDL_asPacketInterval_active_BK = pt.get<std::string>("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_BK", "ref");

			cfg.asPacketInterval.ac[Channels::AC_VI].isRef = (NDL_asPacketInterval_active_VI == "ref"?true:false);
			cfg.asPacketInterval.ac[Channels::AC_VO].isRef = (NDL_asPacketInterval_active_VO == "ref"?true:false);
			cfg.asPacketInterval.ac[Channels::AC_BE].isRef = (NDL_asPacketInterval_active_BE == "ref"?true:false);
			cfg.asPacketInterval.ac[Channels::AC_BK].isRef = (NDL_asPacketInterval_active_BK == "ref"?true:false);

			cfg.asPacketInterval.ac[Channels::AC_VI].val = pt.get("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_VI", NDL_defPacketInterval);
			cfg.asPacketInterval.ac[Channels::AC_VO].val = pt.get("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_VO", NDL_defPacketInterval);
			cfg.asPacketInterval.ac[Channels::AC_BE].val = pt.get("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_BE", NDL_defPacketInterval);
			cfg.asPacketInterval.ac[Channels::AC_BK].val = pt.get("dcc.NDL_asPacketInterval_active"+std::to_string(i)+"_AC_BK", NDL_defPacketInterval);


			//NDL_asDatarate_active
			std::string NDL_asDatarate_active_VI = pt.get<std::string>("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_VI", "ref");
			std::string NDL_asDatarate_active_VO = pt.get<std::string>("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_VO", "ref");
			std::string NDL_asDatarate_active_BE = pt.get<std::string>("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_BE", "ref");
			std::string NDL_asDatarate_active_BK = pt.get<std::string>("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_BK", "ref");

			cfg.asDatarate.ac[Channels::AC_VI].isRef = (NDL_asDatarate_active_VI == "ref"?true:false);
			cfg.asDatarate.ac[Channels::AC_VO].isRef = (NDL_asDatarate_active_VO == "ref"?true:false);
			cfg.asDatarate.ac[Channels::AC_BE].isRef = (NDL_asDatarate_active_BE == "ref"?true:false);
			cfg.asDatarate.ac[Channels::AC_BK].isRef = (NDL_asDatarate_active_BK == "ref"?true:false);

			cfg.asDatarate.ac[Channels::AC_VI].val = pt.get("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_VI", NDL_defDatarate);
			cfg.asDatarate.ac[Channels::AC_VO].val = pt.get("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_VO", NDL_defDatarate);
			cfg.asDatarate.ac[Channels::AC_BE].val = pt.get("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_BE", NDL_defDatarate);
			cfg.asDatarate.ac[Channels::AC_BK].val = pt.get("dcc.NDL_asDatarate_active"+std::to_string(i)+"_AC_BK", NDL_defDatarate);


			//NDL_asCarrierSense_active
			std::string NDL_asCarrierSense_active_VI = pt.get<std::string>("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_VI", "ref");
			std::string NDL_asCarrierSense_active_VO = pt.get<std::string>("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_VO", "ref");
			std::string NDL_asCarrierSense_active_BE = pt.get<std::string>("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_BE", "ref");
			std::string NDL_asCarrierSense_active_BK = pt.get<std::string>("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_BK", "ref");

			cfg.asCarrierSense.ac[Channels::AC_VI].isRef = (NDL_asCarrierSense_active_VI == "ref"?true:false);
			cfg.asCarrierSense.ac[Channels::AC_VO].isRef = (NDL_asCarrierSense_active_VO == "ref"?true:false);
			cfg.asCarrierSense.ac[Channels::AC_BE].isRef = (NDL_asCarrierSense_active_BE == "ref"?true:false);
			cfg.asCarrierSense.ac[Channels::AC_BK].isRef = (NDL_asCarrierSense_active_BK == "ref"?true:false);

			cfg.asCarrierSense.ac[Channels::AC_VI].val = pt.get("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_VI", NDL_defCarrierSense);
			cfg.asCarrierSense.ac[Channels::AC_VO].val = pt.get("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_VO", NDL_defCarrierSense);
			cfg.asCarrierSense.ac[Channels::AC_BE].val = pt.get("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_BE", NDL_defCarrierSense);
			cfg.asCarrierSense.ac[Channels::AC_BK].val = pt.get("dcc.NDL_asCarrierSense_active"+std::to_string(i)+"_AC_BK", NDL_defCarrierSense);

			stateConfig.insert(std::make_pair(i, cfg));
		}

		stateConfig.insert(std::make_pair(NDL_numActiveState+1, DCC_State_Config(NDL_numActiveState+1, NDL_maxChannelLoad, dcc_ALL, NDL_minTxPower, NDL_maxPacketInterval, NDL_maxDatarate, NDL_maxCarrierSense)));

		// set initial ref values by using def values
		NDL_refPacketInterval = NDL_defPacketInterval;
		NDL_refTxPower = NDL_defTxPower;
		NDL_refDatarate = NDL_defDatarate;
		NDL_refCarrierSense = NDL_defCarrierSense;

		assert(fmod(NDL_timeUp, DCC_measure_interval_Tm) == 0);
		assert(fmod(NDL_timeDown, DCC_measure_interval_Tm) == 0);
		assert(NDL_minDccSampling >= DCC_measure_interval_Tm);
	}

};

/**
 * @}
 */

#endif /* DCCCONFIG_H_ */
