/* 
 * File:   spi.h
 * Author: jon
 *
 * Created on 11 November 2013, 14:29
 */

#ifndef SPI_H
#define	SPI_H

#ifdef	__cplusplus
extern "C" {
#endif

void SPI_init();
BYTE SPI_transfer(BYTE data);

#ifdef	__cplusplus
}
#endif

#endif	/* SPI_H */

