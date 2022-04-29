/**
 * @file atComm.cpp
 * @author Simon Latour (simon.latour@cirris.ulaval.ca)
 * @brief 
 * @version 0.1
 * @date 2022-02-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include "atComm.h"
#include "string.h"

atComm::atComm(int  bufferSize)
{

    m_dataBufferSize = bufferSize;
    m_dataBuffer = new uint8_t[m_dataBufferSize];
    m_expectedMessageLen = m_dataBufferSize;

    m_header[0] = HEADER_0;
    if(HEADER_LEN > 0)
        m_header[1] = HEADER_1;
    
    if(HEADER_LEN > 1)
        m_header[2] = HEADER_2;
    
    if(HEADER_LEN > 2)
        m_header[3] = HEADER_3;

    if(HEADER_LEN > 3)
        m_header[4] = HEADER_4;

    resetBuffer();

}

atComm::~atComm()
{
    if (m_dataBuffer) {
		delete[] m_dataBuffer;
		m_dataBuffer = nullptr;
	}

}


    void atComm::lockBuffer()
    {
        m_lockStatus = true;
    }

    void atComm::unlockBuffer()
    {
        m_lockStatus = false;
    }

    bool atComm::getLockStatus()
    {
        return m_lockStatus;
    }


    int atComm::writeHeader()
    {
    	if(m_lockStatus){
				return ATCOMM_ERROR_BUFFER_LOCKED;
			}

        for (int i = 0; i < HEADER_LEN; i++)
        {
            m_dataBuffer[i] = m_header[i];
            m_writePtr++;
        }
        
        m_headerPresent = true;

        return ATCOMM_SUCCESS;
    }

    int atComm::resetBuffer()
    {
        if(m_lockStatus){
            return ATCOMM_ERROR_BUFFER_LOCKED;
        }

        m_writePtr = 0;
        m_headerPresent = false;
        m_packetComplete = false;
        m_expectedMessageLen = m_dataBufferSize; // maximum allowable lenght
        
        for (uint32_t i = 0; i < m_dataBufferSize; i++)
        {
            m_dataBuffer[i] = 0x00;
        }

        return ATCOMM_SUCCESS;

    }


    int atComm::startNewMessage(id_t sourceId, id_t destId)
    {
    	if(m_lockStatus){
			return ATCOMM_ERROR_BUFFER_LOCKED;
		}

        resetBuffer();
        writeHeader();

        //Message length skipped for now
        *(id_t*)(&m_dataBuffer[SOURCE_ID_POS]) = sourceId;
        *(id_t*)(&m_dataBuffer[DESTINATION_ID_POS]) = destId;
        *(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) = 0;
        // Data count skipped for now

        m_writePtr = DATA_START_POS; // ready to write data

        return m_writePtr;
    }

    int atComm::setACKRequest()
	{
    	if(m_lockStatus){
			return ATCOMM_ERROR_BUFFER_LOCKED;
		}

	  //Message length skipped for now
	  *(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) |=  ATCOM_ACK_REQUEST;
	  // Data count skipped for now

	  return ATCOMM_SUCCESS;
	}

    int atComm::setACKStatus()
    {
    	if(m_lockStatus){
			return ATCOMM_ERROR_BUFFER_LOCKED;
		}

    	*(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) |=  ATCOM_ACK;

    	return ATCOMM_SUCCESS;
    }

    int atComm::setNACKStatus()
   {
    	if(m_lockStatus){
    				return ATCOMM_ERROR_BUFFER_LOCKED;
    			}
    	*(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) |=  ATCOM_NACK;

    	return ATCOMM_SUCCESS;
   }

    int atComm::setLastPacketStatus()
    {
    	if(m_lockStatus){
    				return ATCOMM_ERROR_BUFFER_LOCKED;
    			}
    	*(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) |=  ATCOM_LAST_PACKET;

    	return ATCOMM_SUCCESS;
    }


    // buffer transmission functions
    int atComm::addData(dataType_t dataType, dataLenght_t dataLen, void* dataPtr )
    {
    	if(m_lockStatus){
			return ATCOMM_ERROR_BUFFER_LOCKED;
		}

    	if(!m_headerPresent)
        {
            return ATCOMM_ERROR_NOT_INITIALIZED; //Header must be present
        }

        if((m_writePtr + DATA_TYPE_LEN + DATA_LENGTH_LEN + dataLen) > (m_dataBufferSize - CRC_LEN ))
        {
            return ATCOMM_ERROR_NO_ENOUGH_SPACE;
        }

        *(dataType_t*)(&m_dataBuffer[m_writePtr]) = dataType;
        m_writePtr += DATA_TYPE_LEN;
        
        *(dataLenght_t*)(&m_dataBuffer[m_writePtr]) = dataLen;
        m_writePtr += DATA_LENGTH_LEN;

        for(dataLenght_t i = 0; i < dataLen; i++)
        {
            m_dataBuffer[m_writePtr] = ((uint8_t*)dataPtr)[i];
            m_writePtr++;
        }

        *(dataCount_t*)(&m_dataBuffer[DATA_COUNT_POS])   = *(dataCount_t*)(&m_dataBuffer[DATA_COUNT_POS]) + 1; // Increment data count 
        *(messageLen_t*)(&m_dataBuffer[MESSAGE_LENGTH_POS]) +=  DATA_TYPE_LEN+DATA_LENGTH_LEN+dataLen; // Increat total message len ( excluding CRC)

        return (int)m_writePtr;
    }

    int atComm::completeMessage()
    {
        // calculate crc

        if(!m_headerPresent)
        {
            return ATCOMM_ERROR_NOT_INITIALIZED; //Header must be present
        }

        if(m_packetComplete)
        {
            return ATCOMM_ERROR_PACKAGE_ALREADY_COMPLETE;
        }

        crc_t newCRC = calculateCRC(m_dataBuffer, m_writePtr);

        *(crc_t*)(&m_dataBuffer[m_writePtr]) = newCRC;
        m_writePtr+= CRC_LEN;

        m_packetComplete = true;

        return (int)m_writePtr;        
       
    }

    int atComm::getSendPacket(uint8_t* buffer, uint32_t maxLen)
    {
        if(!m_packetComplete)
        {
            return ATCOMM_ERROR_PACKAGE_NOT_COMPLETE;
        }
        
        if((m_writePtr + CRC_LEN) > maxLen)
        {
            return ATCOMM_ERROR_NO_ENOUGH_SPACE;
        }

        uint32_t i;
        for (i = 0; i < (m_writePtr); i++)
        {
            buffer[i] = m_dataBuffer[i];
        }
        
        return i;
    }


    int atComm::getBufferLength()
    {
        return m_writePtr;
    }




    int atComm::generateAckMessage(id_t sourceId, id_t destId)
    {
    	startNewMessage(sourceId, destId);
    	setACKStatus();
    	return completeMessage();
    }

    int atComm::generateNackMessage(id_t sourceId, id_t destId)
    {
    	startNewMessage(sourceId, destId);
    	setNACKStatus();
    	return completeMessage();
    }




    // buffer reception tools
    int atComm::addReceivedBytes(const uint8_t* buffer, int dataLen)
    {
        if(m_writePtr + dataLen > m_dataBufferSize)
        {
            return ATCOMM_ERROR_NO_ENOUGH_SPACE;
        }

        for (int i = 0; i < dataLen; i++)
        {
            if(!m_headerPresent)
            {
                 if(buffer[i] == m_header[m_writePtr])
                 {
                     m_dataBuffer[m_writePtr] = buffer[i];
                     m_writePtr++;
                     if (m_writePtr >= HEADER_LEN) // ALL HEADER HAS BEEN RECEIVED
                     {
                         m_headerPresent = true;
                     }
                 }
                 else if(m_writePtr > 0)
                 {
                    // Receive data do not match header anymore so previosu data was garbage
                    // and we need to reset the buffer.
                    resetBuffer();
                 } 
        
            }
            else    // full header detected so recorde following data
            { 
                m_dataBuffer[m_writePtr] = buffer[i];
                m_writePtr++;
                
                if(m_writePtr == (MESSAGE_LENGTH_POS + MESSAGE_LENGTH_LEN) )
                {
                    // Message lenght received so we know the expected lenght now
                    m_expectedMessageLen =  *(messageLen_t*)&m_dataBuffer[m_writePtr - MESSAGE_LENGTH_LEN] + OVERHEAD_LEN;
                }else
                if(m_writePtr == m_expectedMessageLen )
                {
                        // complete message received, tiem to validate;
                        if( ATCOMM_SUCCESS != validateData())
                        {
                            resetBuffer();
                        }
                }
            }
           
        }

        return (int)m_writePtr;
    }

    // //TODO evaluer pertinence de cette fct
    // int atComm::validHeaderPresent()
    // {
        
    //     for (int i = 0; i < HEADER_LEN; i++)
    //     {
    //         if(m_dataBuffer[i]  != m_header[i])
    //         {
    //             return ATCOMM_ERROR_PACKAGE_NOT_VALID;
    //         }
    //     }
        
    //     m_headerPresent = true;
    //     return ATCOMM_SUCCESS;
    // }
    
    int atComm::validateData()
    {        
        if(!m_headerPresent)
        {
            return ATCOMM_ERROR_PACKAGE_NOT_COMPLETE;
        }

        int startingPos = 0; 

        messageLen_t messageLen = *(messageLen_t*)(&m_dataBuffer[startingPos+HEADER_LEN]);

        if((m_writePtr - startingPos) < (messageLen + FULL_HEADER_LEN + CRC_LEN))
        {
            return ATCOMM_ERROR_PACKAGE_NOT_COMPLETE;
        }

        crc_t newCRC = calculateCRC(&m_dataBuffer[startingPos], messageLen + FULL_HEADER_LEN );

        crc_t receivedCRC = *(crc_t*)(&m_dataBuffer[FULL_HEADER_LEN + messageLen]);
        
        if(newCRC != receivedCRC)
        {
            return ATCOMM_ERROR_PACKAGE_CORRUPTED;
        }

        m_packetComplete = true;

        return ATCOMM_SUCCESS; 
        
    }

    bool atComm::dataAvailable()
    {
        return m_packetComplete;
    }

    int atComm::getDataCount()
    {
        if(!m_packetComplete)
        {
            return ATCOMM_ERROR_PACKAGE_NOT_COMPLETE;
        }

        return (int)*(dataCount_t*)(&m_dataBuffer[DATA_COUNT_POS]);
    }

    int atComm::gatDataInfo(dataCount_t dataNumber, dataInfo_t* dataInfo)
    {

        if(!m_packetComplete)
        {
            return ATCOMM_ERROR_PACKAGE_NOT_COMPLETE;
        }

        if(dataNumber >= getDataCount())
        {
            return ATCOMM_ERROR_INVALID_PARAMETER;
        }

        dataInfo_t tempDataInfo;

        int dataPos = DATA_START_POS;

        for (int i = 0; i <= dataNumber; i++)
        {
            tempDataInfo.dataType = *(dataType_t*)(&m_dataBuffer[dataPos]);
            tempDataInfo.dataLen = *(dataLenght_t*)(&m_dataBuffer[dataPos+DATA_LENGTH_POS]);
            tempDataInfo.dataIndex = dataPos;

            dataPos = dataPos + DATA_TYPE_LEN + DATA_LENGTH_LEN + tempDataInfo.dataLen;// Next Data pos
        }

        *dataInfo = tempDataInfo;

        return ATCOMM_SUCCESS;
    }

    int atComm::getData(dataInfo_t dataInfo, void* buffer, int maxLen)
    {
        dataInfo_t tempDataInfo;

         tempDataInfo.dataType = *(dataType_t*)(&m_dataBuffer[dataInfo.dataIndex]);
         tempDataInfo.dataLen = *(dataLenght_t*)(&m_dataBuffer[dataInfo.dataIndex+DATA_LENGTH_POS]);
         
        if(tempDataInfo.dataLen > maxLen)
        {
            return ATCOMM_ERROR_NO_ENOUGH_SPACE;
        }

        for (int i = 0; i < tempDataInfo.dataLen; i++)
        {
            ((uint8_t*)buffer)[i] = m_dataBuffer[dataInfo.dataIndex + DATA_DATA_POS + i];
        }
        
        return tempDataInfo.dataLen;
    }


crc_t atComm::calculateCRC(uint8_t *data_blk_ptr, uint16_t data_blk_size)
{
  
  crc_t crc_accum = 0;
  uint16_t i;
  static const uint16_t crc_table[256] = {0x0000,
  0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
  0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
  0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D,
  0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
  0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9,
  0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
  0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5,
  0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
  0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
  0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
  0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,
  0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
  0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9,
  0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
  0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176,
  0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
  0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
  0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D,
  0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
  0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A,
  0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
  0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5,
  0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
  0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
  0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
  0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E,
  0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
  0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9,
  0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
  0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5,
  0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
  0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
  0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
  0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E,
  0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
  0x820D, 0x8207, 0x0202 };

  for (uint16_t j = 0; j < data_blk_size; j++)
  {
    i = ((uint16_t)(crc_accum >> 8) ^ *data_blk_ptr++) & 0xFF;
    crc_accum = (crc_accum << 8) ^ crc_table[i];
  }

  return crc_accum;
}


 id_t atComm::getSourceId()
 {
     return *(id_t*)(&m_dataBuffer[SOURCE_ID_POS]);
 }

 id_t atComm::getDestinationId()
 {
     return *(id_t*)(&m_dataBuffer[DESTINATION_ID_POS]);
 }

 messageType_t atComm::getAckStatus()
 {
     return *(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) & 0b00000011;
 }

 bool atComm::getAckRequest()
 {
	 return *(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) & ATCOM_ACK_REQUEST;
 }

 bool atComm::getLastPacketStatus()
 {
	 return *(messageType_t*)(&m_dataBuffer[PACKET_INFO_POS]) & ATCOM_LAST_PACKET;
 }

 int atComm::findHeaderPosition(uint8_t* buffer, int maxLen)
 {

     for (int i = 0; i < maxLen; i++)
     {
        if( 0 ==  memcmp( m_header, &(buffer[i]), HEADER_LEN ) )
        {
            return i;
        }
     }
     
     return -1;
 }
