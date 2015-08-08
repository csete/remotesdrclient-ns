#if !defined( __SDRPROTOCOL_H__ )
#define __SDRPROTOCOL_H__
/* ==========================================================================*/
/* - - - - - - - - -   s d r p r o t o c o l . h   - - - - - - - - - - - - - */
/* ==========================================================================*/
/*    Definition File for the SDR protocol and Control Items                 */
/*  Created 2013-10-02														 */
/*  Modified 2013-10-13  Changed Squelch Threshold Message definition/format */
/*  Modified 2013-12-15  Changed CI_TX_TESTSIGNAL_MODE to avoid conflict     */
/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
#include "interface/ascpmsg.h"	//includes some common ASCP defines

/*---------------------------------------------------------------------------*/
/*----------------------> Control Item Defines <-----------------------------*/
/*---------------------------------------------------------------------------*/

#define CI_GENERAL_OPTIONS 0x000A
 #define CI_GENERAL_OPTIONS_REQLEN 4
 #define CI_GENERAL_OPTIONS_SETRESPLEN 10
  #define GENERAL_CI_GENERAL_OPTION1_SOUND 1		//sound enabled


#define CI_RX_STATE 0x0018
 #define CI_RX_STATE_REQLEN 5
 #define CI_RX_STATE_SETRESPLEN 8
  #define RX_STATE_DATACOMPLEX 0x80		//param 1 channel and data type
  #define RX_STATE_DATAREAL 0x00
  #define RX_STATE_IDLE 0x01			//param 2   run state
  #define RX_STATE_ON 0x02

#define CI_RX_PW_UNLOCK 0x001A
//SET only message that is variable length.
//consists of normal 4 byte header with length and CItem number
// followed by a zero terminated character password string.
//The response to this SET message is the same string if password is accepted
// or a 1 character zero terminated string consisting of the character '0xFF'


//#define CI_RX_FREQUENCY 0x0020
// #define CI_RX_FREQUENCY_REQLEN 5
// #define CI_RX_FREQUENCY_SETRESPLEN 10
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 5 byte value of frequency in Hz.


#define CI_RX_FREQUENCY_REQRANGELEN 5
//Parameter 1 is a 1 byte channel ID.

//#define CI_RX_FREQUENCY_RESPRANGELEN 16+10*n
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte number of ranges.
//Parameter 3 to n are two 5 byte values of frequency min and max in Hz.
//List of frequency ranges each consisting of 10 bytes
//5 byte Frequency min
//5 byte Frequency max

#define CI_RX_DEMOD_MODE 0x0028
 #define CI_RX_DEMOD_MODE_REQLEN 5
 #define CI_RX_DEMOD_MODE_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte value setting the demod mode.
	#define DEMOD_MODE_LSB 0
	#define DEMOD_MODE_USB 1
	#define DEMOD_MODE_DSB 2
	#define DEMOD_MODE_CWL 3
	#define DEMOD_MODE_CWU 4
	#define DEMOD_MODE_FM 5
	#define DEMOD_MODE_AM 6
	#define DEMOD_MODE_SAM 7
	#define DEMOD_MODE_WFM 8
	#define DEMOD_MODE_WAM 9
	#define DEMOD_MODE_DIG 10
	#define DEMOD_MODE_RAW 11
	#define DEMOD_MODE_LAST 11	//edit to make last entry

#define CI_RX_DIGITAL_MODE 0x002A
 #define CI_RX_DIGITAL_MODE_REQLEN 5
 #define CI_RX_DIGITAL_MODE_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte value setting the digital mode.
	#define DIGITAL_MODE_BPSK31 0
	#define DIGITAL_MODE_BPSK63 1
//The following are digital data type codes used in digital daat data packets
#define DIGDATA_TYPE_RXCHAR 0
#define DIGDATA_TYPE_TXECHO 1
#define DIGDATA_TYPE_TXCHAR 0

//#define CI_RX_RF_GAIN 0x0038
// #define CI_RX_RF_GAIN_REQLEN 5
// #define CI_RX_RF_GAIN_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte signed value setting the RF Attenuation in dB(0, -10, -20, -30 only).

#define CI_RX_AF_GAIN 0x0048
 #define CI_RX_AF_GAIN_REQLEN 5
 #define CI_RX_AF_GAIN_SETRESPLEN 6			//param 1 == channel ID
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte value setting the AF Level(0 to 99).

#define CI_RX_AGC 0x0050
 #define CI_RX_AGC_REQLEN 5
 #define CI_RX_AGC_SETRESPLEN 9
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte signed value setting the AGC Threshold in dB(-127 to + 128 dB).
//Parameter 3 is a 1 byte unsigned value setting the AGC Slope in dB(0 to 10 dB).
//Parameter 4 is a 2 byte unsigned value setting the AGC Decay Time in milliseconds(20 to 2000 mSec).

#define CI_RX_DEMOD_FILTER 0x0058
 #define CI_RX_DEMOD_FILTER_REQLEN 5
 #define CI_RX_DEMOD_FILTER_SETRESPLEN 9
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 2 byte unsigned value setting the Low Cutoff Frequency in Hz(100 to 3800 Hz).
//Parameter 3 is a 2 byte unsigned value setting the High Cutoff Frequency in Hz(100 to 3800 Hz).
//Parameter 4 is a 2 byte signed value for the Demodulator Frequency Offset in Hz.(-1000 to +1000Hz)

#define CI_RX_SQUELCH_THRESH 0x0080
 #define CI_RX_SQUELCH_THRESH_REQLEN 5
 #define CI_RX_SQUELCH_THRESH_SETRESPLEN 7
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 2 byte signed 16 bit value for the squelch threshold(0 to -160)dB
 #define CI_RX_SQUELCH_THRESH_MIN -160
 #define CI_RX_SQUELCH_THRESH_MAX 0

#define CI_RX_AUDIO_FILTER 0x0082
 #define CI_RX_AUDIO_FILTER_REQLEN 5
 #define CI_RX_AUDIO_FILTER_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte unsigned value for the audio filter
#define RX_AUDIOFILTER_NONE 0
#define RX_AUDIOFILTER_CTCSS 1

#define CI_RX_AUDIO_COMPRESSION 0x0084
 #define CI_RX_AUDIO_COMPRESSION_REQLEN 5
 #define CI_RX_AUDIO_COMPRESSION_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte unsigned value for the audio comression mode(0 to 4)
//0 == No Audio
//1 == 8 bit raw
//2 == 8 bit 64000bps uLaw G711
//3 == 5 bit 40000bps G726
//4 == 4 bit 32000bps G726
//5 == 3 bit 24000bps G726
//6 == 2 bit 16000bps G726
#define COMP_MODE_NOAUDIO 0
#define COMP_MODE_RAW 1
#define COMP_MODE_G711 2
#define COMP_MODE_G726_40 3
#define COMP_MODE_G726_32 4
#define COMP_MODE_G726_24 5
#define COMP_MODE_G726_16 6
#define COMP_MODE_RAW_16000 7
#define COMP_MODE_RAW_8000 8
#define COMP_MODE_RAW_4000 9
#define COMP_MODE_RAW_2000 10
#define COMP_MODE_RAW_1000 11
#define COMP_MODE_RAW_500 12


#define CI_RX_AD_MODES 0x008A
 #define CI_RX_AD_MODES_REQLEN 5
 #define CI_RX_AD_MODES_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2  is a 1 byte value using bit fields to define various A/D modes
  #define CI_AD_MODES_DITHER 0x01	//bit field defs
  #define CI_AD_MODES_PGA 0x02

#define CI_RX_DOWNCONVERT_SETUP 0x008C			//not used
 #define CI_RX_DOWNCONVERT_SETUP_REQLEN 5
 #define CI_RX_DOWNCONVERT_SETUP_SETRESPLEN 6

#define CI_RX_IN_SAMPLE_RATE 0x00B0
 #define CI_RX_IN_SAMPLE_RATE_REQLEN 5
 #define CI_RX_IN_SAMPLE_RATE_SETRESPLEN 9
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 4 byte input A/D sample rate calibration value.

#define CI_RX_PERFORM_CAL 0x00B2
 #define CI_RX_PERFORM_CAL_REQLEN 5
 #define CI_RX_PERFORM_CAL_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte calibration type value.
  #define RX_PERFORM_CAL_NCONULL 0	//param 2


#define CI_RX_SYNCIN_MODE_PARAMETERS 0x00B4
 #define CI_RX_SYNCIN_MODE_PARAMETERS_REQLEN 5
 #define CI_RX_SYNCIN_MODE_PARAMETERS_SETRESPLEN 8	//p0=chan, p1=mode, p2,3 = number packets
  #define CI_RX_SYNCIN_MODE_OFF 0
  #define CI_RX_SYNCIN_MODE_NEGEDGE 1
  #define CI_RX_SYNCIN_MODE_POSEDGE 2
  #define CI_RX_SYNCIN_MODE_LOWLEVEL 3
  #define CI_RX_SYNCIN_MODE_HIGHLEVEL 4
  #define CI_RX_SYNCIN_MODE_MUTELOW 5
  #define CI_RX_SYNCIN_MODE_MUTEHIGH 6

#define CI_RX_OUT_SAMPLE_RATE 0x00B8
 #define CI_RX_OUT_SAMPLE_RATE_REQLEN 5
 #define CI_RX_OUT_SAMPLE_RATE_SETRESPLEN 9
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 4 byte output sample rate value.

#define CI_RX_OUTPUT_PARAMS 0x00C4
 #define CI_RX_OUTPUT_PARAMS_REQLEN 4
 #define CI_RX_OUTPUT_PARAMS_SETRESPLEN 5

#define CI_RX_UDP_OUTPUT_PARAMS 0x00C5
 #define CI_RX_UDP_OUTPUT_PARAMS_REQLEN 4
 #define CI_RX_UDP_OUTPUT_PARAMS_SETRESPLEN 10

#define CI_RX_CALIBRATION_DATA 0x00D0
 #define CI_RX_CALIBRATION_DATA_REQLEN 5
 #define CI_RX_CALIBRATION_DATA_SETRESPLEN 7	//param 1 == channel ID param 2== 16 bit DC offset

/*  Transmitter Specific Control Items  */

#define CI_TX_PW_UNLOCK 0x011A
//SET only message that is variable length.
//consists of normal 4 byte header with length and CItem number
// followed by a zero terminated character password string.
//The response to this SET message is the same string if password is accepted
// or a 1 character zero terminated string consisting of the character '0xFF'

#define CI_TX_STATE 0x0118
 #define CI_TX_STATE_REQLEN 5
 #define CI_TX_STATE_SETRESPLEN 6
	#define CI_TX_STATE_OFF 0
	#define CI_TX_STATE_ON 1
	#define CI_TX_STATE_DELAYOFF 2


#define CI_TX_FREQUENCY 0x0120
 #define CI_TX_FREQUENCY_REQLEN 5
 #define CI_TX_FREQUENCY_SETRESPLEN 10		//param 1 == channel ID


#define CI_TX_MOD_MODE 0x0128
 #define CI_TX_MOD_MODE_REQLEN 5
 #define CI_TX_MOD_MODE_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte value setting the modulation mode.
	#define MOD_MODE_LSB 0
	#define MOD_MODE_USB 1
	#define MOD_MODE_DSB 2
	#define MOD_MODE_CWL 3
	#define MOD_MODE_CWU 4
	#define MOD_MODE_FM 5
	#define MOD_MODE_AM1 6
	#define MOD_MODE_AM2 7
	#define MOD_MODE_FM2 8
	#define MOD_MODE_AM3 9
	#define MOD_MODE_DIG 10

#define CI_TX_DIGITAL_MODE 0x012A
 #define CI_TX_DIGITAL_MODE_REQLEN 5
 #define CI_TX_DIGITAL_MODE_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte value setting the digital mode.
	//#define DIGITAL_MODE_BPSK31 0
	//#define DIGITAL_MODE_BPSK63 1

#define CI_TX_TESTSIGNAL_MODE 0x0130	//changed 2013-1-15  from 0x012A due to conflict
 #define CI_TX_TESTSIGNAL_MODE_REQLEN 4
 #define CI_TX_TESTSIGNAL_MODE_SETRESPLEN 7
//Parameter 1 is a 1 byte value setting the Tx Audio test signal mode.
	#define TESTSIGNAL_MODE_OFF 0
	#define TESTSIGNAL_MODE_NOISE 1
	#define TESTSIGNAL_MODE_TONE 2
//Parameter 2 is a 2 byte value setting the Tx Audio test Tone Frequency in Hz.

#define CI_TX_EQUALIZER_SETTINGS 0x0144
 #define CI_TX_EQUALIZER_SETTINGS_REQLEN 5
 #define CI_TX_EQUALIZER_SETTINGS_SETRESPLEN 14
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte signed value for Low Shelf Filter Gain(-30dB to 30dB).
//Parameter 3 is a 2 byte unsigned value for Low Shelf Filter Cutoff Frequency in Hz(100 to 1500)
//Parameter 4 is a 1 byte signed value for Peaking Filter Gain(-30dB to 30dB)
//Parameter 5 is a 2 byte unsigned value for Peaking Filter Center Frequency in Hz(100 to 3000)
//Parameter 6 is a 1 byte signed value for High Shelf Filter Gain(-30dB to 30dB)
//Parameter 7 is a 2 byte unsigned value for High Shelf Filter Cutoff Frequency in Hz(1500 to 3000)

#define CI_TX_FM_PARAMS 0x015A
 #define CI_TX_FM_PARAMS_REQLEN 5
 #define CI_TX_FM_PARAMS_SETRESPLEN 9
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 2 byte value for the Peak deviation in Hz.
//Parameter 3 is a 2 byte value for the CTCSS tone frequency in tenths of a Hz.

#define CI_TX_AUDIO_COMPRESSION 0x0184
 #define CI_TX_AUDIO_COMPRESSION_REQLEN 5
 #define CI_TX_AUDIO_COMPRESSION_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte unsigned value for the audio compression mode(0 to 6)
//0 == No Audio
//1 == 8 bit raw
//2 == 8 bit 64000bps uLaw G711
//3 == 5 bit 40000bps G726
//4 == 4 bit 32000bps G726
//5 == 3 bit 24000bps G726
//6 == 2 bit 16000bps G726
//#define COMP_MODE_NOAUDIO 0
//#define COMP_MODE_RAW 1
//#define COMP_MODE_G711 2
//#define COMP_MODE_G726_40 3
//#define COMP_MODE_G726_32 4
//#define COMP_MODE_G726_24 5
//#define COMP_MODE_G726_16 6
//#define COMP_MODE_RAW_16000 7
//#define COMP_MODE_RAW_8000 8
//#define COMP_MODE_RAW_4000 9
//#define COMP_MODE_RAW_2000 10
//#define COMP_MODE_RAW_1000 11
//#define COMP_MODE_RAW_500 12

/* Aux Port Control Items  */
#define CI_OPEN_ASYNC_PORT 0x0200
 #define CI_OPEN_ASYNC_PORT_SETRESPLEN 14
#define CI_CLOSE_ASYNC_PORT 0x0201
 #define CI_CLOSE_ASYNC_PORT_SETRESPLEN 5


#define CI_SPECTRUM_SETTINGS 0x0400
 #define CI_SPECTRUM_SETTINGS_REQLEN 4
 #define CI_SPECTRUM_SETTINGS_SETRESPLEN 16
//Parameter 1  is a 4 byte unsigned value specifying the Span frequency in Hz. (100 to 190000 Hz)
//Parameter 2  is a 2 byte unsigned value specifying the number of data points. (10 to 1024)
//Parameter 3  is a 2 byte signed value specifying the maximum amplitude in dB.( -160 to 0 dB)
//Parameter 4  is a 2 byte signed value specifying the minimum amplitude in dB.( -160 to 0 dB)
//Parameter 5  is a 1 byte value specifying the FFT averaging(1 to 255)
//Parameter 6  is a 1 byte value specifying the FFT Update Frequency in Hz(1 to 10)

#define CI_SPECTRUM_RANGE 0x0404
 #define CI_SPECTRUM_RANGE_REQLEN 4
 #define CI_SPECTRUM_RANGE_SETRESPLEN 20
//Parameter 1 is a 4 byte unsigned value specifying the minimum Receiver Span Frequency in Hz.
//Parameter 2 is a 4 byte unsigned value specifying the maximum Receiver Span Frequency in Hz.
//Parameter 3 is a 4 byte unsigned value specifying the minimum Transmitter Span Frequency in Hz.
//Parameter 4 is a 4 byte unsigned value specifying the maximum Transmitter Span Frequency in Hz.

#define CI_SPECTRUM_AVEPWR 0x0406
 #define CI_SPECTRUM_AVEPWR_REQLEN 4
 #define CI_SPECTRUM_AVEPWR_SETRESPLEN 6

#define CI_VIDEO_COMPRESSION 0x0408
 #define CI_VIDEO_COMPRESSION_REQLEN 4
 #define CI_VIDEO_COMPRESSION_SETRESPLEN 6
//Parameter 1 is a 1 byte video source mode.
#define CVIDEO_MODE_RX 0
#define CVIDEO_MODE_TX 1
#define CVIDEO_MODE_RXTX 2
//Parameter 2 is a 1 byte video spectrum compression method.
 #define COMP_MODE_NOVIDEO 0
 #define COMP_MODE_8BIT 1
 #define COMP_MODE_4BIT 2


/*  Software/Firmware Update MODE Control Items  */
#define CI_UPDATE_MODE_CONTROL 0x0300
 #define CI_UPDATE_MODE_CONTROL_SETREQLEN 10
 #define CI_UPDATE_MODE_CONTROL_SETRESPLEN 10
 #define CI_UPDATE_MODE_CONTROL_ENTER 0
 #define CI_UPDATE_MODE_CONTROL_START 1
 #define CI_UPDATE_MODE_CONTROL_END 2
 #define CI_UPDATE_MODE_CONTROL_ABORT 3
 #define CI_UPDATE_MODE_CONTROL_ERASE 4

#define CI_UPDATE_MODE_PARAMS 0x0302
 #define CI_UPDATE_MODE_PARAMS_SETREQLEN 5
 #define CI_UPDATE_MODE_PARAMS_SETRESPLEN 17

 #define PROG_FLASH_ID 0
 #define CONFIG_FLASH_ID 1

/* Secure Option setup Control Items  */
#define CI_SET_OPTION2 0x0380
 #define CI_SET_OPTION2_REQLEN 4
 #define CI_SET_OPTION2_SETRESPLEN 9
  #define CI_SET_OPTION2_KEY 0x81726354

#define CI_TESTCOMMANDS 0x0400			//Test commands
 #define CI_TESTCOMMANDS_REQLEN 5
 #define CI_TESTCOMMANDS_SETRESPLEN 6


#endif //!defined( __SDRPROTOCOL_H__ )
