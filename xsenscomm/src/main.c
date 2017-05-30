/*
 * main.c
 *
 *	Test various functions in the library
 *
 *  Created on: 2 Nov 2016
 *      Author: Jamie Macaulay
 */


#include "xsensmessage.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static char stringBuffer[100];
ulong audioSampleCount= 100056050;

//int main()
//{
//    union {
//        uint8_t bytes[4];
//        float f;
//    } fun1 = { .bytes = {132,  183 , 53,  63}  }, fun2 = { .bytes = { 0xe1, 0xd1, 0xd7, 0x41} };
//
//    printf("%f\n%f\n", fun1.f, fun2.f);
//
//    return 0;
//}


/**
 * Converts data to a formatted string and saves to spreadsheet.
 */
void dataToString(float* data, int size, char* identifierStr){

	int cx;
	cx=snprintf(stringBuffer, 100, "%lu,", audioSampleCount);	//take a copy for the log function
	cx = snprintf (stringBuffer+cx, 100, "%s,", identifierStr)+cx;
	int a;
	for (a=0; a<size; a++){
		cx = snprintf (stringBuffer+cx, 100, "% .7f,", data[a])+cx;
		//printf("%d\n", cx);
	}
	cx = snprintf (stringBuffer+cx, 100, "\n")+cx;

	printf("%s cx %d\n", stringBuffer, cx);

}


int main(void) {

	printf("Begin test: \n");

	/**********Create an Xbus message**********/
	XBusMessage m;
	m.mid=XMID_MtData2;
	//an example of orientation data.
//	uint8_t exampleMess2[] ={16, 32, 2, 0, 166, 16, 96, 4, 0, 165, 211 ,102 ,32, 16 ,16 ,63, 42, 99 ,137, 58, 169, 20,
//			186 ,59, 116 ,241 ,183, 191 ,63 ,14 ,126 ,224, 32, 4, 0, 0, 0 ,3
//	};

	uint8_t exampleMess2[] ={32,  16,  16,  63,  53,  183 , 132,  186 , 171, 47,  145, 58, 186, 90, 15  ,191 , 52 , 81,  134};

	int a;
	/* for loop execution */
	for( a = 0; a < sizeof(exampleMess2); a++ )
	{
		m.charBufferRx[a]=exampleMess2[a];
	}
	m.len=sizeof(exampleMess2);
	printf("Len: %d \n", m.len);

	/**************Test Functions*************/

	//test data function
	float ori[4];
	if (XbusMessage_getDataItem(ori, XDI_Quaternion, &m))
	{
		printf(" Orientation: (% .7f, % .7f, % .7f, % .7f)\n", ori[0], ori[1],
				ori[2], ori[3]);
	}


	//test raw data function
	uint8_t rawDat[19];
	if (XbusMessage_getDataItemRaw(rawDat, XDI_Quaternion, &m, false))
	{
		int i;
		printf(" Orientation: raw+      ");
		for (i=0; i<16; i++ ){
			printf(" %u",rawDat[i]);
		}
		printf("\n");

	}

	//test raw data function
	if (XbusMessage_getDataItemRaw(rawDat, XDI_Quaternion, &m, true))
	{
		int i;
		printf(" Orientation: raw+ flags");
		for (i=0; i<19; i++ ){
			printf(" %u",rawDat[i]);
		}
		printf("\n");

	}


	XBusMessage m2;
	m2.mid=XMID_MtData2;
	m2.len=14;


	//m2.len=0;

	uint8_t ptExample[] ={8, 16, 4, 65 ,184 ,122, 225, 48 ,32, 4 ,68 ,122, 243,51};

	for( a = 0; a < sizeof(ptExample); a++ )
	{
		m.charBufferRx[a]=ptExample[a];
	}
	m.len=sizeof(ptExample);

	uint8_t* dptr = ptExample;
	uint16_t flag;

	XbusUtility_readU16(&flag, dptr);


	printf("flag %d len %d", flag, m.len);


	if (XbusMessage_getDataItemRaw(rawDat, XDI_Temperature, &m, true))
	{
		uint16_t i;
		printf(" Temperature: %d", m.len);
		for (i=0; i<7; i++ ){
			printf(" %u",rawDat[i]);
		}
		printf("\n");

	}

	float temp[4];
	float* p =temp;

	if (XbusMessage_getDataItem(p, XDI_Temperature, &m))
	{
		printf(" Temperature: %f", temp[0]);
	}

	if (XbusMessage_getDataItem(++p, XDI_Pressure, &m))
	{
		printf(" Pressure: %f\n", temp[1]);
	}

	printf("pointer  %d: \n " ,p-temp);

	p =temp;
	if (XbusMessage_getDataItem(p, XDI_Temperature, &m))
	{
		printf(" Temperature: %f", temp[0]);
	}

	if (XbusMessage_getDataItem(++p, XDI_Pressure, &m))
	{
		printf(" Pressure: %f\n", temp[1]);
	}

	dataToString(ori, 4, "QT");
	dataToString(temp, 2, "PT");

	/***Test output settings configuration***/



	/****Test conversion of 16 bit bytes to 32 bit float*******/

//	uint8_t ptF8[]={ 65,  167,  133 , 31};
//
//	uint16_t ptF16[]={ 65 , 167 , 133 , 31};
//
//	float answer;
//
//	XbusUtility_readF_8(&answer, ptF8);
//
//	printf("Float test 8 but: %f\n", answer);
//
//
//	XbusUtility_readF_16(&answer,ptF16);
//	printf("Float test 16 but: %f\n", answer);
//
//	uint16_t exampleMess3[] ={63,  53,  183 , 132,  186 , 171, 47,  145, 58, 186, 90, 15  ,191 , 52 , 81,  134};
//
//	readFloats16(ori, exampleMess3, 4);


//	//test formatting to send to xsens
//		uint8_t buf[64];
//		size_t rawLength = XbusMessage_format(buf, &m2);
//
//		for (size_t i = 0; i < rawLength; ++i)
//		{
//			printf("%d\n",buf[i]);
//		}
//
//
//		/* an array with 5 elements */
//		uint8_t balance[5] = {1000.0, 2.0, 3.4, 17.0, 50.0};
//		uint8_t* p;
//		int i;
//
//		p = balance;
//
//		/* output each array element's value */
//		printf( "Array values using pointer\n");
//
//		for ( i = 0; i < 5; i++ ) {
//			printf(" %u", p-(balance));
//			*p++;
//		}
//
//
//		return 0;


	return 1;
}
