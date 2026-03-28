





/*-------------------------------------------------------------------------
					Technika Mikroprocesorowa 2 - laboratorium
					Lab 5 - Przetwornik C/A - DDS
					autor: Mariusz Sokołowski
					wersja: 10.09.2025r.
----------------------------------------------------------------------------*/
	
	
#include "MKL05Z4.h"
#include "LCD1602.h"
#include "frdm_bsp.h"
#include <stdio.h>
#include <stdlib.h>

// DEFINICJE PINÓW 
#define BTN_P1 10  // Przycisk 1
#define BTN_P2 12  // Przycisk 2 


#ifndef DELAY
#define DELAY(x) for(volatile int i=0; i<(x)*100; i++)
#endif

volatile uint32_t ms_zegar = 0;
volatile uint8_t kto_klik = 0;
volatile uint8_t idx = 0;

// Tablica probek dla DAC (pila)
uint16_t wave[32] = {0, 132, 264, 396, 528, 660, 792, 924, 1056, 1188, 1320, 1452, 1584, 1716, 1848, 1980,
                     2112, 2244, 2376, 2508, 2640, 2772, 2904, 3036, 3168, 3300, 3432, 3564, 3696, 3828, 3960, 4095};

void SysTick_Handler(void) {
    ms_zegar++;
    if (DAC0->C0 & DAC_C0_DACEN_MASK) {
        DAC0->DAT[0].DATL = (uint8_t)(wave[idx] & 0xFF);
        DAC0->DAT[0].DATH = (uint8_t)((wave[idx] >> 8) & 0x0F);
        idx = (idx + 1) % 32;
    }
}

void PORTA_IRQHandler(void) {
    uint32_t isfr = PORTA->ISFR;
    if (isfr & (1 << BTN_P1)) {
        if (kto_klik == 0) kto_klik = 1;
        PORTA->ISFR |= (1 << BTN_P1);
    }
    if (isfr & (1 << BTN_P2)) {
        if (kto_klik == 0) kto_klik = 2;
        PORTA->ISFR |= (1 << BTN_P2);
    }
}

void My_Init(void) {
    SystemCoreClockUpdate();
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK;
    SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
    
    // Konfiguracja pinów wejściowych
    PORTA->PCR[BTN_P1] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0x0A);
    PORTA->PCR[BTN_P2] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0x0A);
    
    DAC0->C0 &= ~DAC_C0_DACEN_MASK;
    
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
    
    SysTick_Config(SystemCoreClock / 1000);
}

int main(void) {
    char str[16];
    uint8_t runda = 0;
    uint8_t s1 = 0, s2 = 0;
    uint32_t t_ref = 0;
    uint32_t t_rand = 0;
    uint32_t reakcja = 0;

    My_Init();
    LCD1602_Init();
    LCD1602_Backlight(TRUE);

    for (runda = 1; runda <= 5; runda++) {
        LCD1602_ClearAll();
        sprintf(str, "RUNDA %d/5", runda);
        LCD1602_Print(str);
        DELAY(20000);

        kto_klik = 0;
        t_ref = ms_zegar;
        t_rand = 2000 + (rand() % 3000);

        while (ms_zegar - t_ref < t_rand) {
            if (kto_klik != 0) break;
        }

        if (kto_klik != 0) {
            LCD1602_ClearAll();
            sprintf(str, "FALSTART G%d", kto_klik);
            LCD1602_Print(str);
            if (kto_klik == 1) s2++; else s1++;
        } else {
            DAC0->C0 |= DAC_C0_DACEN_MASK;
            t_ref = ms_zegar;
            kto_klik = 0;
            while (kto_klik == 0 && (ms_zegar - t_ref < 5000));
            DAC0->C0 &= ~DAC_C0_DACEN_MASK;

            if (kto_klik == 0) {
                LCD1602_ClearAll();
                LCD1602_Print("CZAS MINAL!");
            } else {
                reakcja = ms_zegar - t_ref;
                LCD1602_ClearAll();
                sprintf(str, "G%d: %u ms", kto_klik, reakcja);
                LCD1602_Print(str);
                if (kto_klik == 1) s1++; else s2++;
            }
        }
        DELAY(30000);
    }

    LCD1602_ClearAll();
    sprintf(str, "G1:%d | G2:%d", s1, s2);
    LCD1602_Print(str);
    LCD1602_SetCursor(0, 1);
    if (s1 > s2) LCD1602_Print("WYGRAL G1!");
    else if (s2 > s1) LCD1602_Print("WYGRAL G2!");
    else LCD1602_Print("REMIS!");

    while(1);
}     