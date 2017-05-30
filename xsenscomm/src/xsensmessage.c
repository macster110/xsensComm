/*
 ============================================================================
 Name        : xsenscomm.c
 Author      : Jamie Macaulay
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "xsensmessage.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//easy converstion between flaots and bytes,
//union u_double
//{
//    uint8_t    bytes[sizeof(double)];
//    float   flt;
//};
//
//union {
//    uint8_t    bytes[sizeof(float)];
//    float   flt;
//} f;

#ifdef SOUNDTRAP_DSP

typedef union {
	uint8_t bytes[4];
	float flt;
} bfloat;

#else

typedef union {
	uint8_t bytes[2];
	float flt;
} bfloat ;

#endif

bfloat f;

/*!
 * \brief Calculate the number of bytes needed for \a message payload.
 */
static uint16_t messageLength(XBusMessage* message)
{
	switch (message->mid)
	{
	case XMID_SetOutputConfig:
		return message->len * 2 * sizeof(uint16_t);

	default:
		return message->len;
	}
}



/*! \brief Write a uint16_t value to an Xbus message. */
uint8_t* XbusUtility_writeU16(uint8_t* out, uint16_t in)
{
	*out++ = (in >> 8) & 0xFF;
	*out++ = in & 0xFF;
	return out;
}


///*! \brief Write a uint32_t value to an Xbus message. */
//uint8_t* XbusUtility_writeU32(uint8_t* out, uint32_t in)
//{
//	*out++ = (in >> 24) & 0xFF;
//	*out++ = (in >> 16) & 0xFF;
//	*out++ = (in >> 8) & 0xFF;
//	*out++ = in & 0xFF;
//	return out;
//}

/*!
 * \brief Read a uint8_t value from an Xbus message.
 */
uint8_t const* XbusUtility_readU8(uint8_t* out, uint8_t const* in)
{
	*out = *in;
	return ++in;
}


/*! \brief Read a uint16_t value from an Xbus message. */
uint8_t const* XbusUtility_readU16(uint16_t* out, uint8_t const* in)
{
	*out = (in[0] << 8) | in[1];
	return in + 2; //don;t use sizeof due to differences in DSP and arduino
}


/*!
 * \brief Read a float value from an Xbus message. This is used when bytes have been read as 16 bit numbers.
 * \param out - pointer to where to set float value output
 * \param in. pointer to bytes bytes in. These are 4 16 bit numbers but with a value between 0 and 255.
 * \return - pointer to the data item in
 */
uint8_t const* XbusUtility_readF(float* out, uint8_t const* in)
{
#ifdef SOUNDTRAP_DSP
	/**
	 * This is a bit of a hack but first combine the 16 bit numbers into to two true 16 bit numbers.
	 * This is because char and int primitives are both 16 bit on SoundTrap DSP.
	 */
	int i;
	for (i = 0; i < 2; ++i)
	{
		//so make one 16 bit into out of two.
		f.bytes[i]=(in[0] << 8) | in[1];
		//printf("bytes %u %u int out %u ", in[0], in[1], f.bytes[i]);
		in++;
		in++;
	}

	//printf("float answer %f \n", f.flt);

	*out=f.flt;

	return in;

#else
	//works on arduino and x86 32/64bit. Does not work on ST DSP
	int i;
	for (i = 0; i < 4; ++i)
	{
		f.bytes[i]=in[3-i];
	}
	*out=f.flt;
	return in + 4;

#endif

}



/*!
 * \brief Read a number of floats from a message payload.
 * \param out Pointer to where to output data.
 * \param raw Pointer to the start of the raw float data.
 * \param floats The number of floats to read.
 */
static void readFloats(float* out, uint8_t const* raw, uint8_t floats)
{
	int i;
	for (i = 0; i < floats; ++i)
	{
		raw = XbusUtility_readF(&out[i], raw);
	}
}


/*!
 * \brief Format the payload of a message from a native data type to
 * raw bytes.
 */
static void formatPayload(uint8_t* raw, XBusMessage* message)
{
	int i;

	switch (message->mid)
	{
	//		case XMID_SetOutputConfig:
	//			formatOutputConfig(raw, message);
	//			break;
	default:
		for (i = 0; i < message->len; ++i)
		{
			*raw++ = ((uint8_t*)message->charBufferRx)[i];
		}
		break;
	}
}

/*!
 * \brief. Format a message to be an output configuration.
 */
bool XBusMessage_setOutPutConfig(OutputConfiguration const* conf, uint8_t elements, XBusMessage* message)
{

	if (elements>ARRAY_SIZE)
	{
		//too many settings for the max message size in this library
		return false;
	}

	message->mid = XMID_SetOutputConfig;
	message->len = elements;

	uint8_t* array = (uint8_t*) conf;
	int i;
	for (i = 0; i < elements; i++) {
		message->charBufferRx[i]= array[i];
	}

	//TODO add some checks
	return true;
}


/*!
 * \brief Format a message into the raw Xbus format ready for transmission to
 * a motion tracker. Calculates the checksum.
 */
int XbusMessage_format(uint8_t* raw, XBusMessage* message)
{
	uint8_t* dptr = raw;

	*dptr++ = XBUS_PREAMBLE;
	*dptr++ = XBUS_MASTERDEVICE;

	uint8_t checksum = (uint8_t)(-XBUS_MASTERDEVICE);

	*dptr = message->mid;
	//printf(" MID %u \n", message->mid);

	checksum -= *dptr++;

	uint16_t length = messageLength(message);
	//printf(" message length %d \n", length);


	if (length < XBUS_EXTENDED_LENGTH)
	{
		*dptr = length;
		checksum -= *dptr++;
	}
	else
	{
		*dptr = XBUS_EXTENDED_LENGTH;
		checksum -= *dptr++;
		*dptr = length >> 8;
		checksum -= *dptr++;
		*dptr = length & 0xFF;
		checksum -= *dptr++;
	}

	formatPayload(dptr, message);
	int i;
	for (i = 0; i < length; ++i)
	{
		checksum -= *dptr++;
	}
	*dptr++ = checksum;

	return dptr - raw;
}


/*!
 * \brief Get a pointer to the data corresponding to \a id.
 * \param id The data identifier to find in the message.
 * \param data Pointer to the raw message payload.
 * \param dataLength The length of the payload in bytes.
 * \returns Pointer to data item, or NULL if the identifier is not present in
 * the message.
 */
static uint8_t const* getPointerToData(enum XsDataIdentifier id, uint8_t const* data, uint16_t dataLength)
{
	uint8_t const* dptr = data;
	while (dptr < data + dataLength)
	{
		uint16_t itemId;
		uint8_t itemSize;

		//printf(" ItemiD, %u %u", dptr[0], dptr[1]);
		dptr = XbusUtility_readU16(&itemId, dptr);
		//		printf(" ItemiD, %d id %d", itemId, id);
		dptr = XbusUtility_readU8(&itemSize, dptr);
		//		printf(" itemSize, %u", itemSize);

		if (id == itemId)
			return dptr;

		dptr += itemSize;
	}


	return NULL;
}


/*!
 * \brief Get the raw data packet for a particular data type
 * \param item Pointer to where to store the raw data packet
 * \param id The data identifier to get.
 * \param message The message to read the data item from.
 * \param flags. Return the raw data with identifier and length bytes. Adds three bytes.
 * \returns true if the data item is found in the message, else false.
 */
bool XbusMessage_getDataItemRaw(uint8_t* item, enum XsDataIdentifier id, XBusMessage* message, bool flags)
{
	uint8_t itemSize;
	uint8_t const* raw = getPointerToData(id, message->charBufferRx, message->len);

	if (raw)
	{
		itemSize=*--raw; //take a peek at the length and then move pointer back again
		raw++;

		//		switch (id)
		//		{
		//		case XDI_PacketCounter:
		//			itemSize=2;
		//			break;
		//		case XDI_SampleTimeFine:
		//		case XDI_StatusWord:
		//			itemSize=4;
		//			break;
		//		case XDI_Quaternion:
		//		case XDI_DeltaQ:
		//			itemSize=16;
		//			break;
		//		case XDI_DeltaV:
		//		case XDI_Acceleration:
		//		case XDI_RateOfTurn:
		//		case XDI_MagneticField:
		//			itemSize=12;
		//			break;
		//		default:
		//			return false;
		//		}


		int n=0;
		if (flags)
		{
			raw=raw-3; //mover pointer back 3
			itemSize=itemSize+3; //increase size of output array (must be right size when input or segmentation fault will occur.
		}

		//now copy from one to the other.
		int i;
		for(i=n; i<itemSize; ++i) {
			item[i] = *(raw++);
		}

		return true;
	}
	else
	{
		return false;
	}
}

/*!
 * \brief Get a data item from an XMID_MtData2 Xbus message.
 * \param item Pointer to where to store the data.
 * \param id The data identifier to get.
 * \param message The message to read the data item from.
 * \returns true if the data item is found in the message, else false.
 */
bool XbusMessage_getDataItem(void* item, enum XsDataIdentifier id, XBusMessage* message)
{
	uint8_t const* raw = getPointerToData(id, message->charBufferRx, message->len);

	if (raw)
	{
		switch (id)
		{
		case XDI_PacketCounter:
			raw = XbusUtility_readU16(item, raw);
			break;
		case XDI_SampleTimeFine:
		case XDI_StatusWord:
			raw = XbusUtility_readF(item, raw);
			break;

		case XDI_Quaternion:
		case XDI_DeltaQ:
			readFloats(item, raw, 4);
			break;

		case XDI_DeltaV:
		case XDI_Acceleration:
		case XDI_RateOfTurn:
		case XDI_MagneticField:
		case XDI_EulerAngles:
			readFloats(item, raw, 3);
			break;
		case XDI_Temperature:
		case XDI_Pressure:
			raw = XbusUtility_readF(item, raw);
			break;
		default:
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}
