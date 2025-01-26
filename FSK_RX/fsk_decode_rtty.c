/*
*   by dl8mcg Jan. 2025
*/

#include <stdint.h>
#include <stdbool.h>

// baudot - tables
const char letters_table[32] =
{
    '\0', 'E', '\n', 'A', ' ', 'S', 'I', 'U',
    '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
    'T', 'Z', 'L', 'W', 'H', 'Y', 'P', 'Q',
    'O', 'B', 'G', ' ', 'M', 'X', 'V', '\0'
};

const char figures_table[32] =
{
    '\0', '3', '\n', '-', ' ', '\'', '8', '7',
    '\r', '$', '4', '\'', ',', '!', ':', '(',
    '5', '+', ')', '2', '#', '6', '0', '1',
    '9', '?', '&', ' ', '.', '/', '=', '\0'
};

enum RTTY_MODE { LETTERS, FIGURES };

// some variables
static enum RTTY_MODE current_mode = LETTERS;
volatile char decoded_char = 0;
volatile bool decready = false;

static uint8_t rxbit;
static uint8_t rxbyte = 0;                      // shift-in register
static uint8_t bit_count = 0;
static uint8_t bit_buffer = 0;

// Funktionszeiger für Zustandsmaschine
static void state1();
static void state2();
static void state3();
static void state4();

static void (*smRtty)() = state1;              // Initialzustand

void process_rtty(uint8_t bit)
{
    rxbyte = (rxbyte << 1) | bit;               // shift in new bit to LSB
    rxbit = bit;                                // Eingehendes Bit speichern
    smRtty();
}

static void state1()                            // Startbit-Suche
{
    if ((rxbyte & 0b111) == 0b110)              // Stopbit, Stopbit, Startbit erkannt
    {
        bit_count = 0;
        bit_buffer = 0;
        smRtty = state2;
    }
}

static void state2()                            // Datenerfassung
{
    bit_buffer = (bit_buffer >> 1) | (rxbit << 4);  // Bits sammeln (LSB zuerst)
    bit_count++;

    if (bit_count == 5)                         // Alle Datenbits gesammelt
    {
        smRtty = state3;
    }
}

static void state3()                            // Prüfung des ersten Stopp-Bits
{
    if (rxbit == 1)
    {
        smRtty = state4;
        return;
    }
    smRtty = state1;
}


static void state4()                            // Prüfung des zweiten Stopp-Bits
{
    if (rxbit == 1)                             // Erstes Stopp-Bit korrekt
    {
        // validate stop-bit (high)
        const char* table = (current_mode == LETTERS) ? letters_table : figures_table;
        if (bit_buffer == 0b11111)              // change to Letters
        {
            current_mode = LETTERS;
        }
        else if (bit_buffer == 0b11011)         // change to Figures
        {
            current_mode = FIGURES;
        }
        else
        {
            decoded_char = table[bit_buffer & 0x1F];  // 5 lsbs
            decready = true;
        }
    }
    smRtty = state1;
}
