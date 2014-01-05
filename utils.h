/* 
 * File:   utils.h
 * Author: jon
 *
 * Created on 14 November 2013
 */

#ifndef UTILS_H
#define	UTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define _XTAL_FREQ 48000000
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))

#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */

