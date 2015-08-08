//////////////////////////////////////////////////////////////////////
// ascpmsg.h: interface/implementation for the CAscpMsg class.
//
// Helper Class to aid in creating and decoding ASCP format msgs
//
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
#ifndef ASCPMSG_H
#define ASCPMSG_H
#include <QtGlobal>

/*---------------------------------------------------------------------------*/
/*---------------------> Message Header Defines <----------------------------*/
/*---------------------------------------------------------------------------*/
#define LENGTH_MASK 0x1FFF			/* mask for message length               */
#define TYPE_MASK 0xE0				/* mask for upper byte of header         */

#define TYPE_HOST_SET_CITEM (0<<5)
#define TYPE_HOST_REQ_CITEM (1<<5)
#define TYPE_HOST_REQ_CITEM_RANGE (2<<5)
#define TYPE_HOST_DATA_ITEM0 (4<<5)
#define TYPE_HOST_DATA_ITEM1 (5<<5)
#define TYPE_HOST_DATA_ITEM2 (6<<5)
#define TYPE_HOST_DATA_ITEM3 (7<<5)

#define TYPE_TARG_RESP_CITEM (0<<5)
#define TYPE_TARG_UNSOLICITED_CITEM (1<<5)
#define TYPE_TARG_RESP_CITEM_RANGE (2<<5)
#define TYPE_TARG_DATA_ITEM0 (4<<5)
#define TYPE_TARG_DATA_ITEM1 (5<<5)
#define TYPE_TARG_DATA_ITEM2 (6<<5)
#define TYPE_TARG_DATA_ITEM3 (7<<5)

#define TYPE_DATA_ITEM_ACK (3<<5)
 #define DATA_ITEM_ACK_LENGTH (3)

/*  2 byte NAK response to any unimplemented messages */
#define TARG_RESP_NAK (0x0002)

#define MAX_ASCPMSG_LENGTH (8192+2)

#define MSGSTATE_HDR1 0		//ASCP msg assembly states
#define MSGSTATE_HDR2 1
#define MSGSTATE_DATA 2

/*  General Interface Control Items  */
#define CI_GENERAL_INTERFACE_NAME 0x0001
 #define CI_GENERAL_INTERFACE_NAME_REQLEN 4

#define CI_GENERAL_INTERFACE_SERIALNUM 0x0002
 #define CI_GENERAL_INTERFACE_SERIALNUM_REQLEN 4

#define CI_GENERAL_INTERFACE_VERSION 0x0003
 #define CI_GENERAL_INTERFACE_VERSION_REQLEN 4
 #define CI_GENERAL_INTERFACE_VERSION_SETRESPLEN 6

#define CI_GENERAL_HARDFIRM_VERSION 0x0004
 #define CI_GENERAL_HARDFIRM_VERSION_REQLEN 5
 #define CI_GENERAL_HARDFIRM_VERSION_SETRESPLEN 7
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_BOOTVER 0
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_APPVER 1
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_HWVER 2
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_CFGVER 3

#define CI_GENERAL_STATUS_CODE 0x0005
 #define CI_GENERAL_STATUS_CODE_REQLEN 4
 #define CI_GENERAL_STATUS_CODE_SETRESPLEN 5
  #define GENERAL_STATUS_IDLE 0x0B
  #define GENERAL_STATUS_BUSY 0x0C
  #define GENERAL_STATUS_6620LOADING 0x0D
  #define GENERAL_STATUS_BOOTIDLE 0x0E
  #define GENERAL_STATUS_BOOTBUSY 0x0F
  #define GENERAL_STATUS_BOOTERROR 0x80
  #define GENERAL_STATUS_ADOVERLOAD 0x20

#define CI_GENERAL_CUSTOM_NAME 0x0008
 #define CI_GENERAL_CUSTOM_NAME_REQLEN 4

#define CI_GENERAL_PRODUCT_ID 0x0009
 #define CI_GENERAL_PRODUCT_ID_REQLEN 4
 #define CI_GENERAL_PRODUCT_ID_SETRESPLEN 8

#define CI_GENERAL_SECURITY_CODE 0x000B
 #define CI_GENERAL_SECURITY_CODE_REQLEN 8
 #define CI_GENERAL_SECURITY_CODE_SETRESPLEN 8

#define CI_GENERAL_FPGA_CONFIG 0x000C
 #define CI_GENERAL_FPGA_CONFIG_REQLEN 4
 #define CI_GENERAL_FPGA_CONFIG_SETQLEN 7
//#define CI_GENERAL_FPGA_CONFIG_SETRESPLEN xx	//variable response length 8 to ?

#define CI_GENERAL_LASTCLIENT_INFO 0x000D
 #define CI_GENERAL_LASTCLIENT_INFO_REQLEN 4
// #define CI_GENERAL_LASTCLIENT_INFO_SETRESPLEN xx  //variable response length to 81

#define CI_RX_FREQUENCY 0x0020
 #define CI_RX_FREQUENCY_REQLEN 5
 #define CI_RX_FREQUENCY_SETRESPLEN 10
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 5 byte value of frequency in Hz.

#define CI_RX_RF_GAIN 0x0038
 #define CI_RX_RF_GAIN_REQLEN 5
 #define CI_RX_RF_GAIN_SETRESPLEN 6
//Parameter 1 is a 1 byte channel ID.
//Parameter 2 is a 1 byte signed value setting the RF Attenuation in dB(0, -10, -20, -30 only).

class CAscpRxMsg
{
public:
	CAscpRxMsg(){Length=0;}
	// For reading msgs
	// Put raw received bytes in Buf8[]
	// Call InitRxMsg before reading msg
	// Call Get type,CItem, Length as needed
	// Call Getparmx() in sequence to read msg parameters in msg order.
	void InitRxMsg(){Length = 4;}
	quint8 GetType(){return Buf8[1]&TYPE_MASK;}
	quint16 GetLength(){return Buf16[0]&LENGTH_MASK;}
	quint16 GetCItem(){return Buf16[1];}
	quint8 GetParm8(){return Buf8[Length++];}
	quint16 GetParm16(){tmp16 = Buf8[Length++]; tmp16 |= (quint16)(Buf8[Length++])<<8;
						return tmp16;	}
	quint32 GetParm32(){tmp32 = Buf8[Length++]; tmp32 |= (quint32)(Buf8[Length++])<<8;
						tmp32 |= (quint32)(Buf8[Length++])<<16;
						tmp32 |= (quint32)(Buf8[Length++])<<24;
						return tmp32;	}
	union
	{
		quint8 Buf8[MAX_ASCPMSG_LENGTH];
		quint16 Buf16[MAX_ASCPMSG_LENGTH/2];
	};
private:
	quint16 Length;
	quint16 tmp16;
	quint32 tmp32;
};

class CAscpTxMsg
{
public:
	CAscpTxMsg(){Length=0;}
	//  For creating msgs to Tx
	// Call InitTxMsg with msg type to start creation
	// Add CItem and parameters, msg length is automatically created in hdr
	void InitTxMsg(quint8 type){Buf16[0] = type<<8; Buf16[0]+=2;}
	void AddCItem(quint16 item){Buf16[1] = item; Buf16[0]+=2;}
	void AddParm8(quint8 parm8){Buf8[ Buf16[0]&LENGTH_MASK ] = parm8; Buf16[0]++;}
	void AddParm16(quint16 parm16){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm16&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm16>>8)&0xFF;Buf16[0]++;}
	void AddParm32(quint32 parm32){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm32&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>8)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>16)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>24)&0xFF;Buf16[0]++;}
	void SetTxMsgLength(quint16 len){Buf16[0]+=len;}
	quint16 GetLength(){return Buf16[0]&LENGTH_MASK;}
	union
	{
		quint8 Buf8[MAX_ASCPMSG_LENGTH];
		quint16 Buf16[MAX_ASCPMSG_LENGTH/2];
	};
private:
	quint16 Length;
};

#endif // ASCPMSG_H
