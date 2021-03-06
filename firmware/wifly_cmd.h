
/*
 Copyright (C) 2012 Nils Weiss, Patrick Bruenn.
 
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

#ifndef _WIFLY_CMD_H_
#define _WIFLY_CMD_H_

#include "platform.h"
#include "rtc.h"
#include "RingBuf.h"
#include "timer.h"
#include "error.h"

#ifdef __cplusplus
#include <ostream>
#endif

//*********************** ENUMERATIONS *********************************************
#define STX 0x0F
#define DLE 0x05
#define ETX 0x04

#define WAIT 0xFE
#define SET_FADE 0xFC
#define SET_GRADIENT 0xF9
#define CLEAR_SCRIPT 0xF8
#define LOOP_ON 0xF7
#define LOOP_OFF 0xF6
#define START_BL 0xF5
#define SET_RTC 0xF4 	
#define GET_RTC 0xF3
#define SET_COLOR_DIRECT 0xF1
#define GET_CYCLETIME 0xF0
#define GET_TRACE 0xEE
#define GET_FW_VERSION 0xED
#define FW_STARTED 0xEC

#define LOOP_INFINITE 0

#define FW_MAX_MESSAGE_LENGTH 128

//*********************** STRUCT DECLARATION *********************************************
struct __attribute__((__packed__)) cmd_set_fade {
	uns8 addr[4];
	uns8 red;
	uns8 green;
	uns8 blue;
	uns8 parallelFade;
	uns16 fadeTmms; //fadetime in ten ms
};

struct __attribute__((__packed__)) cmd_set_gradient {
	uns8 red_1;
	uns8 green_1;
	uns8 blue_1;
	uns8 red_2;
	uns8 green_2;
	uns8 blue_2;
	uns8 parallelAndOffset; //most significant bit is the parallel bit, the 7 lower bit's hold the number of the offset
	uns8 numberOfLeds;
	uns16 fadeTmms; //fadetime in ten ms
	
#ifdef __cplusplus
	uint8_t getOffset(void) const { return parallelAndOffset & 0x7F; };
	void setOffset(const uint8_t offset) { parallelAndOffset = (parallelAndOffset & 0x80) | (offset & 0x7F); };
	bool getParallelFade(void) const { return (parallelAndOffset & 0x80) > 0; };
	void setParallelFade(const bool parallelFade) { parallelAndOffset = (parallelAndOffset & 0x7F) | ( parallelFade ? 0x80 : 0x00); };
#endif
};

struct __attribute__((__packed__)) cmd_loop_end {
	uns8 startIndex; /* pointer to the corresponding cmd_loop_start */
	uns8 counter; /* current loop counter, used due processing */
	uns8 numLoops; /* number of programmed loops f.e. LOOP_INFINITE */
	uns8 depth; /* number of recursions */
};

struct __attribute__((__packed__)) cmd_wait {
	uns16 waitTmms;
};

struct __attribute__((__packed__)) cmd_set_color_direct {
#ifdef __CC8E__
	uns8 ptr_led_array;
#else
	uns8 ptr_led_array[NUM_OF_LED * 3];
#endif
};

struct __attribute__((__packed__)) cmd_get_fw_version {
	uns8 major;
	uns8 minor;
#ifdef __cplusplus
	friend std::ostream& operator<< (std::ostream& out, const cmd_get_fw_version& ref)
	{
		return out << (int) ref.major << '.' << (int) ref.minor << '\n';
	};
#endif
};

struct __attribute__((__packed__)) response_frame {
	uns16 length;		/* only for Firmware, do not use in Client */
	uns8 cmd;
	ErrorCode state;
	union __attribute__((__packed__)) {
		struct rtc_time time;
		struct cmd_get_fw_version version;
		uns8 trace_string[RingBufferSize];
		uns16 max_cycle_times[CYCLETIME_METHODE_ENUM_SIZE];
	}data;
};

struct __attribute__((__packed__)) led_cmd {
	uns8 cmd;
	union {
		struct cmd_set_fade set_fade;
		struct cmd_wait wait;
		struct cmd_loop_end loopEnd;
		struct rtc_time set_rtc;
		struct cmd_set_color_direct set_color_direct;
		struct cmd_set_gradient set_gradient;
	}data;
};

#endif /* #ifndef _WIFLY_CMD_H_ */
