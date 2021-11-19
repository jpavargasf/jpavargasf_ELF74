#include "tx_api.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
//#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

//timer tick per second is around 460. Don't know why.


//retorna o "modo" ou em qual alternativa de escalonamento o programa irá rodar.
//Altere para quaisquer letras abaixo para rodar nos modos definidos abaixo
/*
a) Escalonamento por time-slice de 50 ms. Todas as tarefas com mesma prioridade.

b) Escalonamento sem time-slice e sem preempção. Prioridades estabelecidas no passo 3. A preempção pode ser evitada com o “ 
preemption threshold” do ThreadX.

c) Escalonamento preemptivo por prioridade.

d) Implemente um Mutex compartilhado entre T1 e T3. No início de cada job estas tarefas devem solicitar este mutex e liberá-lo no 
final. Use mutex sem herança de prioridade. Observe o efeito na temporização das tarefas.

e) Idem acima, mas com herança de prioridade
*/
uint8_t modo(){
    //return 'a';
    //return 'b';
    //return 'c';
    //return 'd';
    return 'e';
}


#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_STACK_SIZE         1024

TX_THREAD               thread_1;
TX_THREAD               thread_2;
TX_THREAD               thread_3;
TX_BYTE_POOL            byte_pool_0;
TX_MUTEX                mutex_0;

UCHAR                   byte_pool_memory[DEMO_BYTE_POOL_SIZE];


//ULONG arg_th_1[2];
//ULONG arg_th_2[2];
//ULONG arg_th_3[2];



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

uint8_t nled_to_number(uint8_t nled){
  switch(nled){
  case 1:
    return 8;
  case 2:
    return 4;
  case 3:
    return 2;
  case 4:
    return 1;
  default:
    return 0xFF;
  }
}

uint8_t job_normal(uint8_t n_led,uint32_t n_loop){
    uint32_t counter = 0;
    while(counter<n_loop){
        ledWrite(n_led);
        counter++;
        ledWrite(0);
    }
    return 0;
}

uint8_t job_mutex(uint8_t n_led, uint32_t n_loop){
  if(tx_mutex_get(&mutex_0,TX_WAIT_FOREVER)!=TX_SUCCESS){
      goto job_mutex_fail;
  }
  
  job_normal(n_led,n_loop);
  
  if(tx_mutex_put(&mutex_0)!=TX_SUCCESS){
    job_mutex_fail:return 0xFF; 
  }
  return 0;
}


//talvez eu necessite alterar esta função, para que o led fique acendendo apenas
//ele com leitura e or do resultado, e no final quando o counter ultrapassar
//apaga todos os leds

//Pode parecer que há vários parâmetros de entrada para uma mesma função (e há),
//mas tal foi feito para que se utilize a mesma função para todas as threads em
//todos os items de 'a' a 'e'

//tempo por loop aproximadamente 1,5 us (cronometrado 10s)
//*n é o número de loops que o led acende e apaga
//*(n + 1) é o número do led, que deve ser dentre 1 e 4
//*(n+2) é o número de timer ticks em que a thread fica suspensa (usado em 'b')
//*(n+3) é o job que tal tarefa realiza
//      pode ser simples acender, contar + 1 e apagar o led ou fazer isso mas 
//      com mutex (modo 'd' ou 'e')
void acende_apaga_LED(ULONG input){
    uint32_t *n = (uint32_t*)input;
    
    uint32_t (*job)(uint8_t,uint32_t);
    job = (uint32_t (*)(uint8_t, uint32_t))(*(n+3));
    
    
    //uint32_t counter;
    uint8_t number_to_write;
    
    number_to_write = nled_to_number(*(n+1));
    if(number_to_write == 0xFF){
        return;
    }
 
    while(1){//melhor ter o mutex ser pego antes de fazer o job
      //(job)(number_to_write,counter);
      if((job)(number_to_write,*n) > 0){
          return;
      } 
      /*counter = 0;
      do{
        if((job)(number_to_write,counter) > 0){
            return;
        } 
      }while(++counter<*n);
      */
        tx_thread_sleep(*(n+2));
    }
}


int main()
{
    ledInit();
    tx_kernel_enter();
}
   

void initialize_inputs(ULONG **arg){
    //uint8_t input_size = 4;

    for(int i = 0; i < 3; i++){
        arg[i] = (ULONG*)malloc(4*sizeof(ULONG));
        //numero do led
        arg[i][1] = i + 1;
    }
    
    //tempo de execução das threads, sabendo que cada loop leva 1,5 us
    arg[0][0] = 200000;
    arg[1][0] = 333333;
    arg[2][0] = 533333;
    
    //número de timer ticks que a thread fica suspensa (460 ticks per second)
    arg[0][2] = 322;
    arg[1][2] = 460;
    arg[2][2] = 1472;
    
    //definindo se o job da thread será com mutex ou não
    
    //thread 2 sempre com o job sem mutex
    arg[1][3] = (ULONG)&job_normal;
    
    if(modo() == 'd' || modo() == 'e'){
        arg[0][3] = (ULONG)&job_mutex;
        arg[2][3] = (ULONG)&job_mutex;
    }else{
        arg[0][3] = (ULONG)&job_normal;
        arg[2][3] = (ULONG)&job_normal;
    }
    
}

void    tx_application_define(void *first_unused_memory)
{

CHAR    *pointer = TX_NULL;
    
    //ULONG *arg_th;

#ifdef TX_ENABLE_EVENT_TRACE
    tx_trace_enable(trace_buffer, sizeof(trace_buffer), 32);
#endif

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */
    
    ULONG *arg_th[3];
    initialize_inputs(arg_th);
    
    UINT time_slice;
    
    if(modo() == 'a'){
      time_slice = 23; //50 ms * 460
    }else{
      time_slice = TX_NO_TIME_SLICE;
    }
    
    UINT th_priority[3];
    UINT th_preemp_thresh[3];
    
    for(int i = 0; i < 3; i++){
      if(modo() == 'a'){
        th_priority[i] = 0;
        th_preemp_thresh[i] = 0;
      }else{
        th_priority[i] = i;
        if(modo() == 'b'){
          th_preemp_thresh[i] = 0;
        }else{
          th_preemp_thresh[i] = i;
        }
      }
    }
    
    
    


    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_1, "thread 1", acende_apaga_LED, (ULONG)arg_th[0],  
            pointer, DEMO_STACK_SIZE, 
            th_priority[0], th_preemp_thresh[0], time_slice, TX_AUTO_START);
    
    

    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);


    tx_thread_create(&thread_2, "thread 2", acende_apaga_LED, (ULONG)arg_th[1],  
            pointer, DEMO_STACK_SIZE, 
            th_priority[1], th_preemp_thresh[1], time_slice, TX_AUTO_START);

    


    /* Allocate the stack for thread 2.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_3, "thread 3", acende_apaga_LED, (ULONG)arg_th[2],  
            pointer, DEMO_STACK_SIZE, 
            th_priority[2], th_preemp_thresh[2], time_slice, TX_AUTO_START);
    
    if(modo() == 'd'){
        tx_mutex_create(&mutex_0, "mutex 0", TX_NO_INHERIT);
    }else if(modo() == 'e'){
        tx_mutex_create(&mutex_0, "mutex 0", TX_INHERIT);
    }
}
