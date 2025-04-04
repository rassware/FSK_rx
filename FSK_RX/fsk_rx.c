/*
*   by dl8mcg Jan. 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include "config.h"
#include "fsk_demod.h"
#include "fsk_decode_rtty.h"
#include "sampleprocessing.h"
#include "fsk_decode_ascii.h"
#include "fsk_decode_ax25.h"
#include "buffer.h"

// Funktion zur Abfrage, ob eine Taste gedrückt wurde
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// Funktion zur Zeichenabfrage
int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main() {
    setlocale(LC_ALL, "de_DE.UTF-8");

    wprintf(L"                      RYTL TYTL                                       by dl8mcg 2025\n\n");
    wprintf(L"Mit F1, F2, F3, F4, F5 oder F6 den Modus auswählen              Mit F8 das Programm beenden\n");

    initialize_audiostream();
    init_fsk_demod(FSK_RTTY_45_BAUD_170Hz);

    while (1) {
        if (kbhit()) {
            int key = getch();
            if (key == 27) { // Escape-Sequenz für Funktionstasten beginnt mit 27 (ESC)
                if (getch() == 91) { // '[' folgt auf ESC
                    key = getch(); // eigentliche Taste erfassen

                    switch (key) {
                        case 80: // F1
                            init_fsk_demod(FSK_RTTY_45_BAUD_170Hz);
                            break;
                        case 81: // F2
                            init_fsk_demod(FSK_RTTY_50_BAUD_85Hz);
                            break;
                        case 82: // F3
                            init_fsk_demod(FSK_RTTY_50_BAUD_450Hz);
                            break;
                        case 83: // F4
                            init_fsk_demod(FSK_EFR_200_BAUD_340Hz);
                            break;
                        case 84: // F5
                            init_fsk_demod(FSK_ASCII_300_BAUD_850Hz);
                            break;
                        case 85: // F6
                            init_fsk_demod(FSK_AX25_1200_BAUD_1000Hz);
                            break;
                        case 88: // F8
                            wprintf(L"\n\nProgramm beendet.\n\n");
                            stop_audiostream();
                            wprintf(L"73\n");
                            sleep(1);
                            return 0;
                        default:
                            break;
                    }
                }
            }
        }

        char value;
        if (readbuf(&value)) {
            wprintf(L"%c", value);
        }
    }
}
