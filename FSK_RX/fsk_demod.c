/*
*   by dl8mcg Jan. 2025
*/

#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include "config.h"
#include "fsk_demod.h"
#include "fsk_decode_ascii.h"
#include "fsk_decode_rtty.h"
#include "fsk_decode_ax25.h"

static void (*smMode)(uint8_t) = process_ascii;      // Initialzustand

// NCO parameters
volatile float nco_phase_low = 0.0f;
volatile float nco_phase_high = 0.0f;
float nco_step_low = 0.0f; 
float nco_step_high = 0.0f; 
float baud_rate = 300.0f;
float bit_duration_samples;  
float half_bit_samples;

// Filter coefficients (IIR low-pass filter)
volatile float lpf_alpha = 0;
volatile float i_low = 0, q_low = 0;
volatile float i_high = 0, q_high = 0;

// Synchronization variables
volatile float amplitude_low = 0;
volatile float amplitude_high = 0;
volatile int bit_value = -1;
volatile int previous_bit_value = -1;
volatile float bit_timer = 0;
volatile int demod_bit = 0;


void process_fsk_demodulation(float sample)
{
    // Update NCO for low and high tones
    nco_phase_low += nco_step_low;
    if (nco_phase_low > 2 * (float)M_PI) nco_phase_low -= 2 * (float)M_PI;

    nco_phase_high += nco_step_high;
    if (nco_phase_high > 2 * (float)M_PI) nco_phase_high -= 2 * (float)M_PI;

    // convert FSK signal down to DC with NCOs for high and low tone
    float i_sample_low = sample * cosf(nco_phase_low);
    float q_sample_low = sample * sinf(nco_phase_low);
    float i_sample_high = sample * cosf(nco_phase_high);
    float q_sample_high = sample * sinf(nco_phase_high);

    i_low += lpf_alpha * (i_sample_low - i_low);
    q_low += lpf_alpha * (q_sample_low - q_low);
    i_high += lpf_alpha * (i_sample_high - i_high);
    q_high += lpf_alpha * (q_sample_high - q_high);

    // Compute amplitudes for low- and high - tones
    amplitude_low = sqrtf(i_low * i_low + q_low * q_low);
    amplitude_high = sqrtf(i_high * i_high + q_high * q_high);

    // Detect bit based on amplitude comparison
    bit_value = (amplitude_high > amplitude_low) ? 1 : 0;

    // Edge detection for synchronisation
    if (bit_value != previous_bit_value)
    {
        // Synchronisation on the edge
        bit_timer = half_bit_samples; 
        previous_bit_value = bit_value;
    }

    // bit sampling
    if (--bit_timer <= 0)
    {
        // sample bit and provide
        demod_bit = bit_value;                      

        bit_timer = bit_duration_samples;

        smMode(demod_bit);
    }
}

void init_fsk_demod(FskMode mode) 
{
    float flow;
    float fhigh;

    switch (mode) 
    {
    case FSK_RTTY_45_BAUD:
        baud_rate = 45.454545f;
        lpf_alpha = 0.04f;
        flow = 2125.0f;
        fhigh = 2295.0f;
        smMode = process_rtty;
        wprintf(L"\n\nModus FSK_RTTY_45_BAUD  %g Hz / %g Hz\n\n", flow, fhigh);
        break;

    case FSK_RTTY_50_BAUD:
        baud_rate = 50.0f;
        lpf_alpha = 0.04f;
        flow = 1775.0f;
        fhigh = 2225.0f;
        smMode = process_rtty;
        wprintf(L"\n\nModus FSK_RTTY_50_BAUD  %g Hz / %g Hz\n\n", flow, fhigh);
        break;

    case FSK_ASCII_300_BAUD:
        baud_rate = 300.0f;
        lpf_alpha = 0.04f;
        flow = 1275.0f;
        fhigh = 2125.0f;
        smMode = process_ascii;
        wprintf(L"\n\nModus FSK_ASCII_300_BAUD  %g Hz / %g Hz\n\n", flow, fhigh);
        break;

    case FSK_AX25_1200_BAUD:
        baud_rate = 1200.0f;
        lpf_alpha = 0.1f;
        flow = 1200.0f;
        fhigh = 2200.0f;
        smMode = process_ax25;
        wprintf(L"\n\nModus FSK_AX25_1200_BAUD  %g Hz / %g Hz\n\n", flow, fhigh);
        break;

    default:
        printf("Ungültiger FSK-Modus.\n");
        return;
    }

    nco_step_low = 2 * (float)M_PI * flow / SAMPLING_RATE;
    nco_step_high = 2 * (float)M_PI * fhigh / SAMPLING_RATE;
    bit_duration_samples = SAMPLING_RATE / baud_rate;
    half_bit_samples = bit_duration_samples / 2;
}