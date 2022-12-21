/*
 * xsensmessage.h
 *
 *A light weight C implementation of the xsens library for communication.
 *
 *Created on: 1 Nov 2016
 *Author: Jamie Macaulay
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XSENSMESSAGE_H_
#define XSENSMESSAGE_H_

////define a boolean
//typedef int bool;
//#define true 1
//#define false 0

//define the size of the data buffer.
#define ARRAY_SIZE 255

/*! \brief Xbus message preamble byte. */
#define XBUS_PREAMBLE (0xFA)
/*! \brief Xbus message bus ID for master devices. */
#define XBUS_MASTERDEVICE (0xFF)
/*! \brief Xbus length byte for message with an extended payload. */
#define XBUS_EXTENDED_LENGTH (0xFF)

//#define SOUNDTRAP_DSP

//typedef unsigned char uint8_t;


/*! \brief Xbus message IDs. */
enum XsMessageId
{
  XMID_Wakeup             = 0x3E,
  XMID_WakeupAck          = 0x3F,
  XMID_ReqDid             = 0x00,
  XMID_DeviceId           = 0x01,
  XMID_GotoConfig         = 0x30,
  XMID_GotoConfigAck      = 0x31,
  XMID_GotoMeasurement    = 0x10,
  XMID_GotoMeasurementAck = 0x11,
  XMID_MtData2            = 0x36,
  XMID_ReqOutputConfig    = 0xC0,
  XMID_SetOutputConfig    = 0xC0,
  XMID_OutputConfig       = 0xC1,
  XMID_Reset              = 0x40,
  XMID_ResetAck           = 0x41,
  XMID_Error              = 0x42,
  XMID_ReqPeriodAck       = 0x05,
  XMID_ReqPeriod          = 0x04,
  XMID_ReqBaudRate        = 0x18,
  XMID_ReqBaudRateAck     = 0x19,
  XMID_ReqSTMessage       = 0xD5,
  XMID_ReqSTMessageAck    = 0xD4,
  XMID_SetNoRotation      = 0x22,
  XMID_ReqFilterProfile   = 0x64,
  XMID_ReqFilterProfileAck= 0x65,
  XMID_RestoreFactoryDef  = 0xE,
  XMID_SetRTCTime         = 0x66, 
  XMID_ReqSDFormat        = 0x67,
  XMID_ReqSDSize          = 0x68 
};

/*! \brief Xbus data message type IDs. */
enum XsDataIdentifier
{
  XDI_PacketCounter  = 0x1020,
  XDI_SampleTimeFine = 0x1060,
  XDI_Quaternion     = 0x2010,
  XDI_DeltaV         = 0x4010,
  XDI_Acceleration   = 0x4020,
  XDI_RateOfTurn     = 0x8020,
  XDI_DeltaQ         = 0x8030,
  XDI_MagneticField  = 0xC020,
  XDI_StatusWord     = 0xE020,
  XDI_EulerAngles    = 0x2030,
  XDI_Temperature    = 0x2040, //2064 in decimal - temperature data
  XDI_Pressure       = 0x2050,  //2080 in decimal - pressure data
  XDI_RGB            = 0x2060, //2096 in decimal- red, green, blue data
  XDI_BAT            = 0x2070,   //2112 in decimal- battery data
  XDI_BATV           = 0x2080,   //2112 in decimal- battery data
  XDI_LightSpectrum  = 0x2090,
  XDI_SDCardUsed     = 0x2100
};

//structure to hold message data
typedef struct
 {

  uint8_t bid;

   /*! \brief The message ID of the message. */
   uint8_t mid;

   /*!
  * \brief The length of the payload.
   *
   * \note The meaning of the length is message dependent. For example,
   * for XMID_OutputConfig messages it is the number of OutputConfiguration
   * elements in the configuration array.
   */
   uint16_t len;

   /*!
    * \brief contains all data within a message
   */
   uint8_t charBufferRx[ARRAY_SIZE];

   /**
   * Checksum
   */
   uint8_t checksum;

 } XBusMessage;


 //structure to hold message data
 typedef struct
  {
    /*! \brief the type of data. */
    uint16_t dataid;



    /*!
   * \brief The length of the payload.
    *
    * \note The meaning of the length is message dependent. For example,
    * for XMID_OutputConfig messages it is the number of OutputConfiguration
    * elements in the configuration array.
    */
    uint8_t len;

    /*!
     * \brief contains all data within a message
    */
    uint8_t data[ARRAY_SIZE];
  } XBusData;


int XbusMessage_format(uint8_t* raw, XBusMessage* message);

bool XbusMessage_getDataItemRaw(uint8_t* item, enum XsDataIdentifier id, XBusMessage* message, bool flags);

bool XbusMessage_getDataItem(void* item, enum XsDataIdentifier id, XBusMessage* message);

uint8_t const* XbusUtility_readF(float* out, uint8_t const* in);

uint8_t* XbusUtility_writeU16(uint8_t* out, uint16_t in);

uint8_t const* XbusUtility_readU16(uint16_t* out, uint8_t const* in);


#ifdef __cplusplus
}
#endif // extern "C"

#endif /* XSENSMESSAGE_H_ */
