/*
 nRF24L01 Interface

 created 14 November 2013
 by Jon Nethercott
 */

#include <xc.h>
#include <GenericTypeDefs.h>
#include "utils.h"
#include "spi.h"
#include "nRF24L01.h"

//Pins
#define triscsn TRISA5
#define trisce TRISA4
#define csnPin RA5
#define cePin RA4
//#define irqPin

//Commands
const BYTE R_REG = 0x00;
const BYTE W_REG = 0x20;
const BYTE RX_PAYLOAD = 0x61;
const BYTE TX_PAYLOAD = 0xA0;
const BYTE FLUSH_TX = 0xE1;
const BYTE FLUSH_RX = 0xE2;
const BYTE ACTIVATE = 0x50;
const BYTE R_STATUS = 0xFF;

//Registers
const BYTE NRF_CONFIG = 0x00;
const BYTE EN_AA = 0x01;
const BYTE EN_RXADDR = 0x02;
const BYTE SETUP_AW = 0x03;
const BYTE SETUP_RETR = 0x04;
const BYTE RF_CH = 0x05;
const BYTE RF_SETUP = 0x06;
const BYTE NRF_STATUS = 0x07;
const BYTE OBSERVE_TX = 0x08;
const BYTE CD = 0x09;
const BYTE RX_ADDR_P0 = 0x0A;
const BYTE RX_ADDR_P1 = 0x0B;
const BYTE RX_ADDR_P2 = 0x0C;
const BYTE RX_ADDR_P3 = 0x0D;
const BYTE RX_ADDR_P4 = 0x0E;
const BYTE RX_ADDR_P5 = 0x0F;
const BYTE TX_ADDR = 0x10;
const BYTE RX_PW_P0 = 0x11;
const BYTE RX_PW_P1 = 0x12;
const BYTE RX_PW_P2 = 0x13;
const BYTE RX_PW_P3 = 0x14;
const BYTE RX_PW_P4 = 0x15;
const BYTE RX_PW_P5 = 0x16;
const BYTE FIFO_STATUS = 0x17;
const BYTE DYNPD = 0x1C;
const BYTE FEATURE = 0x1D;

//Data
BYTE RXTX_ADDR[3] = { 0xB5, 0x23, 0xA5 };  //Randomly chosen address
BOOL rfCardPresent = FALSE;

//Local Helper Function Prototypes
void FlushTXRX();
void WriteRegister(BYTE reg, BYTE val);
void WriteAddress(BYTE reg, BYTE num, BYTE* addr);
BYTE ReadRegister(BYTE reg);
BYTE ReadStatus();
void WriteCommand(BYTE command);
void WritePayload(BYTE num, BYTE* data);
void ReadPayload(BYTE num, BYTE* data);


void nRF_Setup()
{
  // start the SPI library:
  SPI_init();

  // initalize the  CSN and CE pins:
  triscsn = 0;
  trisce = 0;

  csnPin = 1;
  cePin = 0;

  WriteRegister(NRF_CONFIG, 0x0B);     //1 BYTE CRC, POWER UP, PRX
  WriteRegister(EN_AA, 0x00);          //Disable auto ack
  WriteRegister(EN_RXADDR, 0x01);      //Enable data pipe 0
  WriteRegister(SETUP_AW, 0x01);       //3 BYTE address
  WriteRegister(SETUP_RETR, 0x00);     //Retransmit disabled
  WriteRegister(RF_CH, 0x01);          //Randomly chosen RF channel
  WriteRegister(RF_SETUP, 0x26);       //250kbps, 0dBm
  WriteRegister(RX_PW_P0, 0x01);       //RX payload = 1 BYTE

  WriteAddress(RX_ADDR_P0, 3, RXTX_ADDR);
  WriteAddress(TX_ADDR, 3, RXTX_ADDR);

  FlushTXRX();

  if ((ReadRegister(NRF_CONFIG) & 0x08) != 0)
      rfCardPresent = TRUE;
}

void RXMode()
{
  WriteRegister(NRF_CONFIG, 0x0B);         //1 BYTE CRC, POWER UP, PRX
  cePin = 1;
  //According to the datasheet we shouldn't bring CSN low within Tpece2csn
  //after setting ce high. Can't see why (or when that would happen though)
  //so comment out the next line.
  //__delay_us(4);    //Tpece2csn
}

void TXMode()
{
  cePin = 0;
  WriteRegister(NRF_CONFIG, 0x0A);         //1 BYTE CRC, POWER UP, PTX
}

void PowerDown()
{
  cePin = 0;
  WriteRegister(NRF_CONFIG, 0);
}

BYTE RXChar()
{
  BYTE data;
  ReadPayload(1, &data);
  //Clear status bit
  WriteRegister(NRF_STATUS, 0x40);
  return data;
}

void TXChar(BYTE ch)
{
  WritePayload(1, &ch);

  if (rfCardPresent)
  {
      //Wait for char to be sent
      BYTE stat;
      do
      {
          stat = ReadStatus();
      } while ((stat & 0x20) == 0);
  }

  //Clear status bit
  WriteRegister(NRF_STATUS, 0x20);
}

BOOL ReadDataAvailable()
{
  BYTE stat = ReadStatus();
  return (stat & 0x40) != 0;
}

void FlushTXRX()
{
  WriteRegister(NRF_STATUS, 0x70);    //Clear: data RX ready, data sent TX, Max TX retransmits
  WriteCommand(FLUSH_RX);
  WriteCommand(FLUSH_TX);
}

// *************** Helper Methods ***************

void WriteRegister(BYTE reg, BYTE val)
{
  csnPin = 0;
  SPI_transfer(W_REG | reg);
  SPI_transfer(val);
  csnPin = 1;
}

//Address is 3-5 bytes, LSB first
void WriteAddress(BYTE reg, BYTE num, BYTE* addr)
{
  csnPin = 0;
  SPI_transfer(W_REG | reg);
  for (BYTE i=0; i<num; i++)
    SPI_transfer(addr[i]);
  csnPin = 1;
}

BYTE ReadRegister(BYTE reg)
{
  csnPin = 0;
  SPI_transfer(R_REG | reg);
  BYTE val = SPI_transfer(0x00);
  csnPin = 1;
  return val;
}

BYTE ReadStatus()
{
  csnPin = 0;
  BYTE val = SPI_transfer(R_STATUS);
  csnPin = 1;
  return val;
}

void WriteCommand(BYTE command)
{
  csnPin = 0;
  SPI_transfer(command);
  csnPin = 1;
}

void WritePayload(BYTE num, BYTE* data)
{
  csnPin = 0;
  SPI_transfer(TX_PAYLOAD);
  for (BYTE i=0; i<num; i++)
    SPI_transfer(data[i]);
  csnPin = 1;

  cePin = 1;
  __delay_us(12);   //Thce (10us) + a bit (2us)
  cePin = 0;
}

void ReadPayload(BYTE num, BYTE* data)
{
  csnPin = 0;
  SPI_transfer(RX_PAYLOAD);
  for (BYTE i=0; i<num; i++)
    data[i] = SPI_transfer(0);
  csnPin = 1;
}


