/**
 * @file atComm.h
 * @author Simon Latour (simon.latour@cirris.ulaval.ca)
 * @brief 
 * @version 0.1
 * @date 2022-02-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __ATCOMM_H__
#define __ATCOMM_H__


#include "stdint.h"

#define __UNIT_TEST__


#ifdef __UNIT_TEST__
    #define private public
#endif


#define ATCOMM_ERROR_BUFFER_LOCKED -1




typedef uint8_t atCommId_t;
typedef uint8_t atCommPaquetInfo_t;

typedef uint16_t dataCount_t;
typedef uint16_t messageLen_t;

typedef uint8_t dataType_t;
typedef uint16_t dataLenght_t;

typedef uint16_t crc_t;

typedef struct 
{
    uint32_t dataIndex;
    dataType_t dataType;
    dataLenght_t dataLen;
}dataInfo_t;


#define HEADER_0    85
#define HEADER_1    'A'
#define HEADER_2    'T'
#define HEADER_3    'C'
#define HEADER_4    58

//GLOBAL POSITION
#define HEADER_POS          0
#define HEADER_LEN          5
#define MESSAGE_LENGTH_POS     (HEADER_POS + HEADER_LEN)
#define MESSAGE_LENGTH_LEN     sizeof(messageLen_t)//2

#define SOURCE_ID_POS       (MESSAGE_LENGTH_POS + MESSAGE_LENGTH_LEN)
#define SOURCE_ID_LEN       sizeof(atCommId_t)
#define DESTINATION_ID_POS  (SOURCE_ID_POS + SOURCE_ID_LEN)
#define DESTINATION_ID_LEN  sizeof(atCommId_t)
#define PACKET_INFO_POS		(DESTINATION_ID_POS + DESTINATION_ID_LEN)
#define PACKET_INFO_LEN		sizeof(atCommPaquetInfo_t)
#define DATA_COUNT_POS		(PACKET_INFO_POS + PACKET_INFO_LEN)
#define DATA_COUNT_LEN      sizeof(dataCount_t)//2
#define DATA_START_POS      (DATA_COUNT_POS + DATA_COUNT_LEN)

#define FULL_HEADER_LEN (DATA_START_POS)

#define CRC_LEN         sizeof(crc_t)

#define OVERHEAD_LEN (FULL_HEADER_LEN + CRC_LEN) 

//Message types
// [b7 b6 b5 b4]	DATA
// [b3]				Final message indicator bit
// [b2]				ack request
// [b1 b0]			Ack status

#define ATCOM_ACK 					0b01000000
#define ATCOM_NACK					0b10000000
#define ATCOM_ACK_REQUEST 			0b00100000
#define ATCOM_LAST_PACKET			0b00010000

#define PACKET_INFO_TYPE_UNDEFINED	0x00
//#define PACKET_INFO_TYPE_ACK_ONLY	0x01
//#define PACKET_INFO_TYPE_DATA		0x02
//#define PACKET_INFO_FIRST_PACKET	0x03
//#define PACKET_INFO_LAST_PACKET		0x04

//DATA packet 
#define DATA_TYPE_POS       0
#define DATA_TYPE_LEN       sizeof(dataType_t)//2
#define DATA_LENGTH_POS     (DATA_TYPE_POS + DATA_TYPE_LEN)
#define DATA_LENGTH_LEN     sizeof(dataLenght_t)//2
#define DATA_DATA_POS       DATA_LENGTH_POS + DATA_LENGTH_LEN

//Error status
#define ATCOMM_SUCCESS                          0
#define ATCOMM_ERROR_NO_ENOUGH_SPACE            -1
#define ATCOMM_ERROR_NOT_INITIALIZED            -2

#define ATCOMM_ERROR_PACKAGE_ALREADY_COMPLETE   -10
#define ATCOMM_ERROR_PACKAGE_NOT_COMPLETE       -11
#define ATCOMM_ERROR_PACKAGE_NOT_VALID          -12
#define ATCOMM_ERROR_PACKAGE_CORRUPTED          -13

#define ATCOMM_ERROR_INVALID_PARAMETER          -20        



class atComm
{
    private:

		uint32_t m_dataBufferSize;
        uint8_t *m_dataBuffer;

        bool m_lockStatus = false;
        bool m_packetComplete = false;
        bool m_headerPresent = false;
        uint32_t m_expectedMessageLen;

        uint8_t m_header[HEADER_LEN];

        uint32_t m_writePtr = 0;

        int writeHeader();

        crc_t calculateCRC(uint8_t *data_blk_ptr, uint16_t data_blk_size);
   
    public: 
   

        atComm(int  bufferSize);
        ~atComm();

        //General functions
        void lockBuffer();          //test done
        void unlockBuffer();        //test done
        bool getLockStatus();       //test done
        int resetBuffer();          //done
        int validateData();         //done
        

        // buffer transmission functions
        int startNewMessage(atCommId_t sourceId, atCommId_t destId);    //done
        int setACKRequest(); //TODO test
        int setACKStatus(); //TODO test
        int setNACKStatus(); //TODO test
        int setLastPacketStatus(); //TODO test
        int addData(dataType_t dataType, dataLenght_t dataLen, void* dataPtr ); //done
        int completeMessage();   //done
        int getSendPacket(uint8_t* buffer, uint32_t maxLen);
        int getBufferLength();

        // Shortcut functions
        int generateAckMessage(atCommId_t sourceId, atCommId_t destId);
        int generateNackMessage(atCommId_t sourceId, atCommId_t destId);

        // buffer reception tools
        int addReceivedBytes(const uint8_t* buffer, int dataLen);
        //int validHeaderPresent();   
        bool dataAvailable();

        int getDataCount();
        int getDataInfo(dataCount_t dataNumber, dataInfo_t* dataInfo); // done
        int getData(dataInfo_t dataInfo, void* buffer,int maxLen);      //done

        atCommId_t getSourceId();         //done
        atCommId_t getDestinationId();    //done
        atCommPaquetInfo_t getAckStatus(); 	//TODO test
        bool getAckRequest(); 	//TODO test
        bool getLastPacketStatus();

        //External tools to facilitate integration
        int findHeaderPosition(uint8_t* buffer, int maxLen); //
        int getMessageBufferLen(uint8_t* buffer, int headerPos, int maxLen); //

};









#endif
