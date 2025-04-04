/*
*   by dl8mcg Jan. 2025       2FSK AX25 - Decoder frame detection
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "fsk_demod.h"
#include "config.h"
#include "buffer.h"

// state machine frame
static void stateframe0();
static void stateframe1();
static void stateframe2();
static void (*smAX25)() = stateframe0;       // function pointer for state machine

// state machine content
static void statecontent0();
static void statecontent1();
static void statecontent2();
static void statecontent3();
static void (*smContent)() = statecontent0;  // function pointer for state machine

static uint8_t rxbyte = 0;              // shift-in register
static uint8_t oldbit = 0;              // old bit
static uint8_t bitcnt = 0;              // bit counter
static uint8_t onecnt = 0;              // counter for one-bits
static bool rxbit = false;              // received bit

static uint32_t rxword = 0;
static int stopcnt = 0;

void process_ax25(uint8_t bit)
{
    if (bit == oldbit)                  // differential decoding: no change = 1, change = 0
        rxbit = 1;
    else
        rxbit = 0;
    oldbit = bit;

    if ((onecnt == 5) && (rxbit == 0))
    {
        onecnt = 0;
        return;
    }

    rxbyte = (rxbyte >> 1) | (rxbit << 7);     // shift in new rxbit to MSB

    if (rxbit == 1)
        onecnt++;
    else
        onecnt = 0;

    if (onecnt > 7)                      // error when more than 7 one-bits
    {
        onecnt = 0;
        smAX25 = stateframe0;
        return;
    }

    smAX25();
}

void stateframe0()
{
    rxword = (rxword << 1) | (rxbit);     // shift in new rxbit to LSB

    if (rxword == 0x7E7E7E7E)               // start flag detected
    {
        bitcnt = 0;
        smAX25 = stateframe1;
    }
}

void stateframe1()
{
    bitcnt++;

    if (rxbyte == 0x7E)                 // an additional start flag detected
    {
        bitcnt = 0;
        return;                         // Stay in state1
    }

    if (bitcnt < 8)                     // wait for 8 data bits
        return;

    bitcnt = 0;                         // reset bit-counter

    smContent = statecontent0;
    smContent();

    smAX25 = stateframe2;
}

void stateframe2()
{
    bitcnt++;

    if (rxbyte == 0x7E)                 // stop flag detected
    {
        bitcnt = 0;                     // reset bit-counter
        smAX25 = stateframe1;           // back to state1
        return;
    }

    if (bitcnt == 8)                    // payload databyte received
    {
        bitcnt = 0;                     // reset bit-counter
        smContent();
    }
}

void statecontent0()
{
    writebuf(rxbyte >> 1);

    if (rxbyte & 0x01)
    {
        smContent = statecontent1;
        return;
    }
}

void statecontent1()
{
    if (!(rxbyte & 0x01))
    {
        smContent = statecontent2;          // I-frame
        return;
    }
    if ((rxbyte & 0x03) == 0x01)
    {
        smContent = statecontent3;          // S-frame
        return;
    }
    smContent = statecontent3;              // U-frame
    return;
}

void statecontent2()
{
    smContent = statecontent3;
    return;
}

void statecontent3()
{
    writebuf(rxbyte);
    smContent = statecontent3;
    return;
}

