/*
 Copyright (C) 2012, 2013 Nils Weiss, Patrick Bruenn.
 
 This file is part of Wifly_Light.
 
 Wifly_Light is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Wifly_Light is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Wifly_Light.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _FwResponse_h
#define _FwResponse_h

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include "wifly_cmd.h"
#include "WiflyControlException.h"

class FwResponse
{
public:
	virtual bool Init(response_frame& frame, size_t dataLength) = 0;
	virtual ~FwResponse() {};
protected:
	FwResponse(void) {};
};

class SimpleResponse : public FwResponse
{
public:
	SimpleResponse(uint8_t cmd) : mCmd(cmd) {};

	/*
	 * Validate and convert data from response_frame
	 * @return true, if convertion was successfull, false if data was corrupted and a retry might be successfull.
	 * @throw FatalError if command code of the response doesn't match the code of the request
	 * @throw ScriptBufferFull if script buffer in PIC firmware is full and request couldn't be executed
	 */
	bool Init(response_frame& pData, const size_t dataLength)
	{
		if(dataLength < 4) {
			// response to short -> seems corrupted, allow retry
			return false;
		}

		if(mCmd != pData.cmd) {
			// response doesn't match to request, allow retry
			return false;
		}

		switch(pData.state)
		{
			case OK:
				return true;
			case SCRIPTBUFFER_FULL:
				throw ScriptBufferFull{};
			case CRC_CHECK_FAILED:
			case BAD_PACKET:
				return false;
			case BAD_COMMAND_CODE:
				throw FatalError("FIRMWARE RECEIVED A BAD COMMAND CODE" + pData.cmd);
			default:
				throw FatalError("Unexpected response state: " + pData.state);
		};
	};
	
private:
	const uint8_t mCmd;
};

class RtcResponse : public SimpleResponse
{
public:
	RtcResponse(void) : SimpleResponse(GET_RTC) {};
	bool Init(response_frame& pData, size_t dataLength)
	{
		if(SimpleResponse::Init(pData, dataLength)
		&& (dataLength >= 4 + sizeof(struct rtc_time)))
		{
			mTimeValue.tm_sec = pData.data.time.tm_sec;
			mTimeValue.tm_min = pData.data.time.tm_min;
			mTimeValue.tm_hour = pData.data.time.tm_hour;
			mTimeValue.tm_mday = pData.data.time.tm_mday;
			mTimeValue.tm_year = pData.data.time.tm_year;
			mTimeValue.tm_wday = pData.data.time.tm_wday;
			mTimeValue.tm_mon = pData.data.time.tm_mon;
			return true;
		}
		return false;
	};
	struct tm GetRealTime(void) const {return mTimeValue; };
	
private:
	struct tm mTimeValue;
};

class CycletimeResponse : public SimpleResponse
{
public:
	CycletimeResponse(void) : SimpleResponse(GET_CYCLETIME) {};
	bool Init(response_frame& pData, size_t dataLength)
	{
		if(SimpleResponse::Init(pData, dataLength)
		&& (dataLength >= 4 + sizeof(mCycletimes[0]) * CYCLETIME_METHODE_ENUM_SIZE))
		{
			for(size_t i = 0; i < CYCLETIME_METHODE_ENUM_SIZE && i < dataLength / sizeof(uns16); i++)
			{
				mCycletimes[i] = ntohs(pData.data.max_cycle_times[i]);
			}
			return true;
		}
		return false;
	};

	std::string ToString(void) const
	{
		std::stringstream stream;
		stream << *this;
		std::string temp;
		temp = stream.str();
		return temp;
	};

	friend std::ostream& operator<< (std::ostream& out, const CycletimeResponse& ref)
	{
		out << "Cycletimes: \n";
		for( unsigned int i = 0; i < CYCLETIME_METHODE_ENUM_SIZE; i++)
		{
			out << std::setw(3) << std::dec << i + 1 << ": " << std::setw(8) << std::dec << ref.mCycletimes[i] << " us\n";
		}
		return out;
	};

private:
	uint16_t mCycletimes[CYCLETIME_METHODE_ENUM_SIZE];
};

class TracebufferResponse : public SimpleResponse
{
public:
	TracebufferResponse(void) : SimpleResponse(GET_TRACE) {};
	bool Init(response_frame& pData, size_t dataLength)
	{
		if(SimpleResponse::Init(pData, dataLength))
		{
			mTraceMessage = std::string((char*)pData.data.trace_string, dataLength - 4);
			return true;
		}
		return false;
	};

	std::string ToString(void) const
	{
		std::stringstream stream;
		stream << *this;
		std::string temp;
		temp = stream.str();
		return temp;
	};

	friend std::ostream& operator<< (std::ostream& out, const TracebufferResponse& ref)
	{
		out << "Tracebuffercontent: " << ref.mTraceMessage;
		return out;
	};
	
private:
	std::string mTraceMessage;
};

class FirmwareVersionResponse : public SimpleResponse
{
public:
	FirmwareVersionResponse(void) : SimpleResponse(GET_FW_VERSION) {};
	bool Init(response_frame& pData, size_t dataLength)
	{
		if(SimpleResponse::Init(pData, dataLength)
		&& (dataLength >= 4 + sizeof(struct cmd_get_fw_version)))
		{
			mFwVersion = pData.data.version;
			return true;
		}
		return false;
	};

	std::string ToString(void) const
	{
		std::stringstream stream;
		stream << *this;
		std::string temp;
		temp = stream.str();
		return temp;
	};

	friend std::ostream& operator<< (std::ostream& out, const FirmwareVersionResponse& ref)
	{
		return out << "Firmwareversion: " << std::dec << ref.mFwVersion << std::endl;
	};
	
private:
	cmd_get_fw_version mFwVersion;
};
#endif
