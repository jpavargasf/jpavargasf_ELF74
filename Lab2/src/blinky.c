//*****************************************************************************
//
// blinky.c - Simple example to blink the on-board LED.
//
// Copyright (c) 2013-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.2.0.295 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

//#include "inc/hw_ints.h"
//#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include <stdio.h>
#include <stdlib.h>
//#include <time.h>

#define CLK 20000000  
//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Blinky (blinky)</h1>
//!
//! A very simple example that blinks the on-board LED using direct register
//! access.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1);
}
#endif

//*****************************************************************************
//
// Blink the on-board LED.
//
//*****************************************************************************

volatile uint8_t ui8WarpCount;
volatile uint8_t ui8LedOn;

//interrupçã para o botão
void intButton(void);

//escreve um número em binário nos leds
void ledWrite(uint8_t ui8Number){
    //bits 3 e 2
    GPIOPinWrite(GPIO_PORTN_BASE,(GPIO_PIN_0 | GPIO_PIN_1), (GPIO_PIN_0 | GPIO_PIN_1) & (ui8Number>>2));
    //bits 0 e 1
    GPIOPinWrite(GPIO_PORTF_BASE,(GPIO_PIN_0 | GPIO_PIN_4), (GPIO_PIN_0 & ui8Number) | (GPIO_PIN_4 & (ui8Number<<3)));
}
//inicialização dos 4 leds
void ledInit(void){
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)||SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4 );
    ledWrite(0);
}

//configura o botão e interrupção
void buttonInit(uint8_t ui8Pins, void (*funPtr)(void)){
    //confere se é um valor válido
    if((ui8Pins&3)==0)  return;
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,ui8Pins);

    //configura pull up resistor
    GPIOPadConfigSet(GPIO_PORTJ_BASE,ui8Pins,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
    
    //define função de interrupção
    GPIOIntRegister(GPIO_PORTJ_BASE,funPtr);
    
    //define em que modo a interrupção é gerada
    GPIOIntTypeSet(GPIO_PORTJ_BASE,ui8Pins ,GPIO_LOW_LEVEL);
    
    //habilita interrupção
    GPIOIntEnable(GPIO_PORTJ_BASE,ui8Pins );
}

void GPIOChangeState(uint32_t ui32Port,uint8_t ui8Pins){
    GPIOPinWrite(ui32Port,ui8Pins,~GPIOPinRead(ui32Port,ui8Pins));
}

//configura o SysTick, para ter interrupção a cada clockNumber clocks
void configureSysTick(uint32_t clockNumber, void (*funPtr)(void)){
    SysTickPeriodSet(clockNumber);
    SysTickIntEnable();
    SysTickIntRegister(funPtr);
    SysTickEnable();
}

//função de atendimento à interrupção do botão
void intButton(void){
    uint32_t ui32CLKReaction = SysTickValueGet() + (CLK/2)*ui8WarpCount;
    GPIOIntClear(GPIO_PORTJ_BASE,GPIO_INT_PIN_0);
    if(ui8LedOn==0)       return;
    printf("Clock: %d Hz\nNumero de Clocks de reacao : %d clocks\n",CLK,ui32CLKReaction);
    ui8LedOn = 0;
    ledWrite(0);   
    return;
}

//função de atendimento à interrupção do SysTick
void intSysTick(void){
    if(ui8LedOn==1){
        if(++ui8WarpCount!=6)   return;
        ledWrite(0);
        ui8LedOn=0;
        printf("Tempo limite atingido\n");
    }
}


int
main(void)
{
    //srand(time(NULL));
    ui8LedOn=0;
    ui8WarpCount=0;
    
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |SYSCTL_OSC_MAIN |SYSCTL_USE_PLL |SYSCTL_CFG_VCO_240), CLK);    
    buttonInit((uint8_t)1,intButton);
    ledInit();
    configureSysTick(CLK/2,intSysTick);
    ledWrite(8);
    ui8LedOn=1;
    while(1);
}
