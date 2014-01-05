/*
 nRF24L01 Header

 created 14 November 2013
 by Jon Nethercott
 */

void nRF_Setup();
void RXMode();
void TXMode();
void PowerDown();
BYTE RXChar();
void TXChar(BYTE ch);
BOOL ReadDataAvailable();
void FlushTXRX();
