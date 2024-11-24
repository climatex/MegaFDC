// Bogin: added XMODEM-1K packet size
// This code was taken from: https://github.com/mgk/arduino-xmodem
// (https://code.google.com/archive/p/arduino-xmodem)
// which was released under GPL V3:
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "XModem.h"
const unsigned char XModem::NACK = 21;
const unsigned char XModem::ACK =  6;
const unsigned char XModem::SOH =  1;
const unsigned char XModem::STX =  2;
const unsigned char XModem::EOT =  4;
const unsigned char XModem::CAN =  0x18;

const int XModem::m_receiveDelay=7000;
const int XModem::m_rcvRetryLimit = 10;


XModem::XModem(int (*recvCharFn)(int msDelay),
               void (*sendDataFn)(const char *data, int len),
               bool (*dataHandlerFn)(unsigned long number, char *buffer, int len),
               bool XMODEM_1K)
{
	sendData = sendDataFn;
	recvChar = recvCharFn;
	dataHandler = dataHandlerFn;
  
  m_blockSize = XMODEM_1K ? 1024 : 128;    
  m_buffer = new char[m_blockSize + 3 + 2]; // 3 head + 2 CRC
  
  // not enough memory for 1K?
  if (!m_buffer)
  {
    m_blockSize = 128;
    m_buffer = new char[m_blockSize + 3 + 2];
  }
}

XModem::~XModem()
{
  if (m_buffer)
  {
    delete[] m_buffer;
  }
}

bool XModem::dataAvail(int delay)
{
	if (m_byte != -1)
		return true;
	if ((m_byte = recvChar(delay)) != -1)
		return true;
	else
		return false;
		
}
int XModem::dataRead(int delay)
{
	int b;
	if(m_byte != -1)
	{
		b = m_byte;
		m_byte = -1;
		return b;
	}
	return recvChar(delay);
}
void XModem::dataWrite(char symbol)
{
  sendData(&symbol, 1);
}
bool XModem::receiveFrameNo()
{
	unsigned char num = 
		(unsigned char)dataRead(XModem::m_receiveDelay);
	unsigned char invnum = 
		(unsigned char)dataRead(XModem::m_receiveDelay);
	m_repeatedBlock = false;
	//check for repeated block
	if (invnum == (255-num) && num == m_blockNo-1) {
		m_repeatedBlock = true;
		return true;	
	}
	
	if(num !=  m_blockNo || invnum != (255-num))
		return false;
	else
		return true;
}
bool XModem::receiveData()
{
	if (!m_buffer)
    return false;
  
  for(int i = 0; i < m_blockSize; i++) {
		int byte = dataRead(XModem::m_receiveDelay);
		if(byte != -1)
			m_buffer[i] = (unsigned char)byte;
		else
			return false;
	}
	return true;	
}
bool XModem::checkCrc()
{
  if (!m_buffer)
    return false;
  unsigned short frame_crc = ((unsigned char)
				dataRead(XModem::m_receiveDelay)) << 8;
	
	frame_crc |= (unsigned char)dataRead(XModem::m_receiveDelay);
	//now calculate crc on data
	unsigned short crc = crc16_ccitt(m_buffer, m_blockSize);
	
	if(frame_crc != crc)
		return false;
	else
		return true;
	
}
bool XModem::checkChkSum()
{
  if (!m_buffer)
    return false;
  unsigned char frame_chksum = (unsigned char)
						dataRead(XModem::m_receiveDelay);
	//calculate chksum
	unsigned char chksum = 0;
  
	for(int i = 0; i< m_blockSize; i++) {
		chksum += m_buffer[i];
	}
	if(frame_chksum == chksum)
		return true;
	else
		return false;
}
bool XModem::sendNack()
{
	dataWrite(XModem::NACK);	
	m_retries++;
	if(m_retries < XModem::m_rcvRetryLimit)
		return true;
	else
		return false;
	
}
bool XModem::receiveFrames(transfer_t transfer)
{
 if (!m_buffer)
    return false;
  bool handlerOk = true;
	m_blockNo = 1;
	m_blockNoExt = 1;
	m_retries = 0;
	while (1) {
		char cmd = dataRead(1000);
		switch(cmd){
			case XModem::SOH:
      case XModem::STX:
				if (!receiveFrameNo()) {
					if (sendNack())
						break;
					else
						return false;
				}
				if (!receiveData()) {	
					if (sendNack())
						break;
					else
						return false;
					
				};
				if (transfer == Crc) {
					if (!checkCrc()) {
						if (sendNack())
							break;
						else
							return false;
					}
				} else {
					if(!checkChkSum()) {
						if (sendNack())
							break;
						else
							return false;
					}
				}
				//callback
				if(handlerOk && dataHandler != NULL && m_repeatedBlock == false)
                                  handlerOk = dataHandler(m_blockNoExt, m_buffer, m_blockSize);
				//ack
                                if( handlerOk )
                                  {
                                    dataWrite(XModem::ACK);
                                    if(m_repeatedBlock == false)
                                      {
					m_blockNo++;
					m_blockNoExt++;
                                      }
                                    m_retries = 0;
                                  }
                                else { dataWrite(XModem::CAN); dataWrite(XModem::CAN); dataWrite(XModem::CAN); return true; }

				break;
			case XModem::EOT:
				dataWrite(XModem::ACK);
				return true;
			case XModem::CAN:
				//wait second CAN
				if(dataRead(XModem::m_receiveDelay) ==
						XModem::CAN) {
					dataWrite(XModem::ACK);
					//flushInput();
					return false;
				}
				//something wrong
				dataWrite(XModem::CAN);
				dataWrite(XModem::CAN);
				dataWrite(XModem::CAN);
				return false;
			default:
				//something wrong
				dataWrite(XModem::CAN);
				dataWrite(XModem::CAN);
				dataWrite(XModem::CAN);
				return false;
		}
		
	}
}
void XModem::init()
{
	//set preread m_byte  	
	m_byte = -1;
}
bool XModem::receive()
{
	init();
	
	for (int i =0; i <  m_blockSize; i++)
	{
		dataWrite('C');	
		if (dataAvail(1000)) 
			return receiveFrames(Crc);
	
	}
	for (int i =0; i <  m_blockSize; i++)
	{
		dataWrite(XModem::NACK);	
		if (dataAvail(1000)) 
			return receiveFrames(ChkSum);
	}
  return false;
}
unsigned short XModem::crc16_ccitt(char *buf, int size)
{
	unsigned short crc = 0;
	while (--size >= 0) {
		int i;
		crc ^= (unsigned short) *buf++ << 8;
		for (i = 0; i < 8; i++)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc <<= 1;
	}
	return crc;
}
unsigned char XModem::generateChkSum(const char *buf, int len)
{
	//calculate chksum
	unsigned char chksum = 0;
	for(int i = 0; i< len; i++) {
		chksum += buf[i];
	}
	return chksum;
	
}

bool XModem::transmitFrames(transfer_t transfer)
{
  if (!m_buffer)
    return false;
  m_blockNo = 1;
	m_blockNoExt = 1;
	// use this only in unit tetsing
	//memset(m_buffer, 'A', m_blockSize);
	while(1)
	{
		//get data
		if (dataHandler != NULL)
		{
			if( false == 
			    dataHandler(m_blockNoExt, m_buffer+3, 
			    m_blockSize))
			{
				//end of transfer
				dataWrite(XModem::EOT);
				//wait ACK
				if (dataRead(XModem::m_receiveDelay) == 
					XModem::ACK)
					return true;
				else
					return false;

			}			
			
		}
		else
		{
			//cancel transfer - send CAN twice
			dataWrite(XModem::CAN);
			dataWrite(XModem::CAN);
			//wait ACK
			if (dataRead(XModem::m_receiveDelay) == 
				XModem::ACK)
				return true;
			else
				return false;
		}
		//SOH / STX
    m_buffer[0] = (m_blockSize == 1024) ? XModem::STX : XModem::SOH;
		//frame number
		m_buffer[1] = m_blockNo;
		//inv frame number
		m_buffer[2] = (unsigned char)(255-(m_blockNo));
		//(data is already in buffer starting at byte 3)
		//checksum or crc
		if (transfer == ChkSum) {
                  m_buffer[3+m_blockSize] = generateChkSum(m_buffer+3, m_blockSize);
                  sendData(m_buffer, 3+m_blockSize+1);
		} else {
                  unsigned short crc;
                  crc = crc16_ccitt(m_buffer+3, m_blockSize);
                  m_buffer[3+m_blockSize+0] = (unsigned char)(crc >> 8);
                  m_buffer[3+m_blockSize+1] = (unsigned char)(crc);;
                  sendData(m_buffer, 3+m_blockSize+2);
		}

		//TO DO - wait NACK or CAN or ACK
		int ret = dataRead(XModem::m_receiveDelay);
		switch(ret)
		{
			case XModem::ACK: //data is ok - go to next chunk
				m_blockNo++;
				m_blockNoExt++;
				continue;
			case XModem::NACK: //resend data
				continue;
			case XModem::CAN: //abort transmision
				return false;

		}
	
	}
	return false;
}
bool XModem::transmit()
{
	int retry = 0;
	int sym;
	init();
	
	//wait for CRC transfer
	while(retry < 256)
	{
		if(dataAvail(1000))
		{
			sym = dataRead(1); //data is here - no delay
			if(sym == 'C')	
				return transmitFrames(Crc);
			if(sym == XModem::NACK)
				return transmitFrames(ChkSum);
		}
		retry++;
	}	
	return false;
}
