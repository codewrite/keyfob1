/* 
 * File:   main.c
 * Author: jon
 *
 * Created on 14 November 2013, 10:02
 */

#include <xc.h>
#include <GenericTypeDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include "nRF24L01.h"
#include "utils.h"

//Make sure the ports are set up to reflect the following:
#define SWITCH1 RA3
#define SWITCH2 RA1
#define SWITCH3 RA0
#define LED_RED RC5
#define LED_YELLOW RC4
#define LED_GREEN RC3

#define SET_LED(x) x = 1; asm("nop")
#define CLR_LED(x) x = 0; asm("nop")

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = SWDTEN    // Watchdog Timer Enable (WDT controlled by the SWDTEN bit in the WDTCON register)
#pragma config PWRTE = OFF       // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config CPUDIV = NOCLKDIV // CPU System Clock Selection Bit (CPU system clock divided by 1)
#pragma config USBLSCLK = 48MHz // USB Low SPeed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
#pragma config PLLMULT = 3x     // PLL Multipler Selection Bit (3x Output Frequency Selected)
#pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = ON       // Low-Power Brown Out Reset (Low-Power BOR is enabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (Low-voltage programming disabled)

//Local Helper Function Prototypes
void Initialize();
BOOL ReadChar(BOOL relay);
void Shutdown();


int timerCount = 0;             //in tenths of a second

BYTE counter = 0;
BOOL switchOn = FALSE;
int ledTimeout = 0;

/*
 * PIC16F1455 has 11 I/O. 6 are used for SPI, this leaves 5 for switches and LEDs
 * PIC16F1459 has 17 I/O. 6 are used for SPI, this leaves 11 for switches and LEDs.
 */
void main()
{
    BOOL wasSwitch3 = FALSE;
    
    Initialize();

    while (1)
    {
        Shutdown();
        __delay_ms(20);     //Debounce switches
        wasSwitch3 = FALSE;
        if (SWITCH1 == 0)
        {
            TXChar('a');
            SET_LED(LED_RED);
        }
        if (SWITCH2 == 0)
        {
            TXChar('b');
            SET_LED(LED_YELLOW);
        }
        if (SWITCH3 == 0)
        {
            TXChar('c');
            SET_LED(LED_GREEN);
            wasSwitch3 = TRUE;
        }
        RXMode();
        while(timerCount < 3)      //Flash selected LED for a tenth of a second
        {
            asm("SLEEP");
            timerCount++;
        }
        CLR_LED(LED_RED);
        CLR_LED(LED_YELLOW);
        CLR_LED(LED_GREEN);
        BOOL charRec = FALSE;
        timerCount = 0;
        while(timerCount < 33)      //Wait for 1 second before shutting down
        {
            asm("SLEEP");
            if (ReadChar(FALSE))
            {
                charRec = TRUE;
            }
            timerCount++;
        }
        
        if (wasSwitch3 && (SWITCH3 == 0))
        {
            BOOL gotoReceiveMode = TRUE;
            //Switch 3 still pressed - wait a further 3 seconds
            //and if still pressed go into receive mode
            timerCount = 0;
            while(timerCount < 99)      //Wait further 3 seconds
            {
                asm("SLEEP");
                if (SWITCH3 != 0)
                {
                    gotoReceiveMode = FALSE;
                    break;
                }
                timerCount++;
            }
            if (gotoReceiveMode)
            {
                int readTimestamp = -1;
                RXMode();
                timerCount = 0;
                while(timerCount < 1875)      //Loop for 1 minute
                {
                    asm("SLEEP");
                    if (SWITCH1 == 0)          //Shutdown immediately if SW1 pressed
                    {
                        __delay_ms(1000);     //delay before shutting down
                        break;
                    }
                    if (ReadChar(TRUE))
                    {
                        readTimestamp = timerCount;
                    }
                    else
                    {
                        if (readTimestamp >= 0)
                        {
                            if ((timerCount - readTimestamp) >= 33)
                            {
                                //Turn receive LED off after 1 second
                                CLR_LED(LED_RED);
                                CLR_LED(LED_YELLOW);
                                CLR_LED(LED_GREEN);
                                readTimestamp = -1;
                            }
                        }
                        else
                        {
                            BYTE tick = timerCount % 33;
                            if (tick == 0)
                            {
                                SET_LED(LED_GREEN);
                            }
                            else if (tick == 1)
                                CLR_LED(LED_GREEN);
                        }
                    }

                    timerCount++;
                }
            }
        }
        
        TXMode();
        CLR_LED(LED_RED);
        CLR_LED(LED_YELLOW);
        CLR_LED(LED_GREEN);
        if (!charRec)
        {
            SET_LED(LED_RED);
            SET_LED(LED_YELLOW);
            SET_LED(LED_GREEN);
            asm("SLEEP");      //Flash all LEDs for 32ms
            CLR_LED(LED_RED);
            CLR_LED(LED_YELLOW);
            CLR_LED(LED_GREEN);
        }
    }

    while (1)
    {
        Shutdown();
        __delay_ms(20);     //Debounce switches
        if (SWITCH1 == 0)
            SET_LED(LED_RED);
        if (SWITCH2 == 0)
            SET_LED(LED_YELLOW);
        if (SWITCH3 == 0)
            SET_LED(LED_GREEN);
        while(timerCount < 33)
        {
            asm("SLEEP");
            timerCount++;
        }
        CLR_LED(LED_RED);
        CLR_LED(LED_YELLOW);
        CLR_LED(LED_GREEN);
    }

    __delay_ms(500);
    char rxChar = 0; //SendChar('b');
    switch (rxChar)
    {
        case 'a':
            SET_LED(LED_RED);
            ledTimeout = 500;	//Half a second
            break;
        case 'b':
            SET_LED(LED_YELLOW);
            ledTimeout = 500;	//Half a second
            break;
        case 'c':
            SET_LED(LED_GREEN);
            ledTimeout = 500;	//Half a second
            break;
        case 0:
            SET_LED(LED_RED);
            SET_LED(LED_YELLOW);
            SET_LED(LED_GREEN);
            ledTimeout = 100;	//Half a second
            break;
    }

    for (int i=0; i<ledTimeout; i++)
        __delay_ms(1);
    CLR_LED(LED_RED);
    CLR_LED(LED_YELLOW);
    CLR_LED(LED_GREEN);

    __delay_ms(500);
    while (1)
    {
        SET_LED(LED_RED);
        __delay_ms(500);
        CLR_LED(LED_RED);
        SET_LED(LED_YELLOW);
        __delay_ms(500);
        CLR_LED(LED_YELLOW);
        SET_LED(LED_GREEN);
        __delay_ms(500);
        CLR_LED(LED_GREEN);
    }


    __delay_ms(2000);

    while(1)
    {
        __delay_ms(500);
        RA5 = ~RA5;
    }

    while(1)
    {
        counter++;
        if ((counter & 0x01) == 0)
            RA5 = ~RA5;
        asm("lpsleep2:");
        asm("SLEEP");
        asm("BTFSC PORTA, 0x04");
        asm("BRA lpsleep2");

	//__delay_ms(4);
    }
}

void Initialize()
{
    OSCCON = 0xFC;          //16MHz HFINTOSC with 3x PLL enabled (48MHz operation)
    VREGCON = 0x03;         //Enable low power sleep mode

    nWPUEN = 0;		    //Global weak pull-ups enabled
    TRISC5 = 0;             //LED on RC5, set as output
    TRISC4 = 0;             //LED on RC4, set as output
    TRISC3 = 0;             //LED on RC3, set as output
    ANSA4 = 0;              //Switch on RA4, set as input

    SET_LED(LED_RED);
    CLR_LED(LED_YELLOW);
    CLR_LED(LED_GREEN);
    // give the nRF24L01 time to power up:
    __delay_ms(100);
    nRF_Setup();
    __delay_ms(5);    //Tpd2stby
    TXMode();
    CLR_LED(LED_RED);
}

BOOL ReadChar(BOOL relay)
{
  char rch = 0;
  if (ReadDataAvailable())
      rch = RXChar();
  switch (rch)
  {
      case 'a':
          SET_LED(LED_RED);
          break;
      case 'b':
          SET_LED(LED_YELLOW);
          break;
      case 'c':
          SET_LED(LED_GREEN);
          break;
      case 0:
          break;    //do nothing if no char received
      default:
          SET_LED(LED_RED);
          SET_LED(LED_YELLOW);
          SET_LED(LED_GREEN);
          break;
  }
  if (relay && (rch != 0))
  {
      TXMode();
      TXChar(rch);
      RXMode();
  }
  return rch != 0;
}

void Shutdown()
{
    BYTE oldA = TRISA;
    BYTE oldC = TRISC;
    //TRISA = 0xff;
    //TRISC = 0xff;

    PowerDown();
    IOCAN = 0x0b;
    IOCIE = 1;              //Enable interrupt on change
    IOCAF = 0;              //Clear IOC flags
    WDTCON = 0x00;          //Disable watchdog
    asm("SLEEP");
    //Startup code, caused by button press
    //TRISA = oldA;
    //TRISC = oldC;
    TXMode();
    WDTCON = 0x0b;          //32ms watchdog
    IOCIE = 0;              //Disable interrupt on change
    IOCAN = 0x00;
    timerCount = 0;
}

