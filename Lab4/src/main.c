#include <stdint.h>
#include <stdbool.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
//#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"


//as definições abaixo são porque tais não são acessadas no arquivo pin_map.h
//isso pode ser resolvido definindo manualmente a placa no início do programa
#define GPIO_PA0_U0RX           0x00000001
#define GPIO_PA1_U0TX           0x00000401

#define BUFFER_SIZE 4

uint8_t ui8ByteBuffer[BUFFER_SIZE];
uint8_t ui8Position;
uint8_t ui8LastPosition;


//tratamento da interrupção 
void UART0_Handler(void){
  ui8ByteBuffer[ui8Position%BUFFER_SIZE] = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
  ui8Position++;
  UARTIntClear(UART0_BASE,UARTIntStatus(UART0_BASE,true));
}

void uart0_init(){
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  //ver pin_map.h
  //GPIO_PA0_U0RX
  //GPIO_PA1_U0TX
  
  //UART tem que estar desabilitada antes de mexer na config
  //UARTDisable(UART0_BASE);
  
  //pino 0 RX
  GPIOPinConfigure(GPIO_PA0_U0RX);
  //pino 1 TX
  GPIOPinConfigure(GPIO_PA1_U0TX);
  
  GPIOPinTypeUART(GPIO_PORTA_BASE,(GPIO_PIN_0|GPIO_PIN_1));

  //a funç a seguir invoca tanto disable e enable, que habilita FIFO
  UARTConfigSetExpClk(UART0_BASE,(uint32_t)120000000,(uint32_t)115200,(UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE));
  
  //se desabilitar a FIFO, a interrupção de recebimento ocorrerá após receber 1 byte
  //caso contrário, o mínimo são dois bytes com FIFO enabled
  UARTFIFODisable(UART0_BASE);
  
  //habilita interrupção para recebimento e 
  UARTIntEnable(UART0_BASE,UART_INT_RX|UART_INT_RT);
  UARTIntRegister(UART0_BASE,UART0_Handler);

}

//transforma letras maiúsculas em minúsculas
uint8_t char_process(uint8_t *ui8Char){
  if(((*ui8Char)>=(uint8_t)65)&&((*ui8Char)<=(uint8_t)90)){
    *ui8Char += 32; 
  }
  return *ui8Char;
}

void uart_process(){
  //se não há entrada nova
  if(ui8LastPosition==ui8Position)      return;
  char_process(&ui8ByteBuffer[ui8LastPosition%BUFFER_SIZE]);
  //poderia ser UARTCharPut(...) que garante o envio do char, mas em uma só
  //chamada de função poderia gastar muito tempo para enviar o char 
  if(UARTCharPutNonBlocking(UART0_BASE,ui8ByteBuffer[ui8LastPosition%BUFFER_SIZE])){
    ui8LastPosition++;
  }
}

int main()
{
  ui8Position = 0;
  ui8LastPosition = 0;
  uart0_init();
  
  while(1){
    //por polling ->teria só que desabilitar a interrupção
    //UARTCharPut(UART0_BASE,char_process(&UARTCharGet(UART0_BASE)));
    uart_process();
  }
}
