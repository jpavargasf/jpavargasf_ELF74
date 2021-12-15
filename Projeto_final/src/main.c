//Autor:João Paulo Vargas da Fonseca
//Comentários: No arquivo tx_initialize_low_level foi alterado o clocl source 
//para 120 MHz
//
//Apesar de no documento 2, versão 10-Dez-2021 ter um grupo de flags na arquite-
//tura física, elas não foram implementadas. A ideia era delas sinalizarem o iní-
//cio da operação para os elevadores e também para ajudar na requisição de posição,
//mas o sinal de inicialização foi feito enviando uma mensagem do distribuidor
//para os elevadores e a reqyusulção de posição não foi implementada então as flags
//não foram implementadas
//
//Comentário Adicional sobre algo NÃO IMPLEMENTADO:
//Uma alternativa para realmente fazer o controle em malha fechada seria ter mais
//uma thread responsável pela verificação possíveis erros nos elevadores, ela 
//ficaria responsável pela requisição de posição caso após um tempo os elevadores
//não enviarem sinais à mesma.
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
//#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "tx_api.h"




//as definições abaixo são porque tais não são acessadas no arquivo pin_map.h
//isso pode ser resolvido definindo manualmente a placa no início do programa
#define GPIO_PA0_U0RX           0x00000001
#define GPIO_PA1_U0TX           0x00000401


//define o numero dos elevadores
#define CENTRAL 0
#define DIREITA 1
#define ESQUERDA 2
#define TODOS 3

//definindo para a porta ficar aberta 2s ( 100 ticks por segundo)
#define TICKS_PORTA_ABERTA 200

//threads dos elevadores
//0 = central
//1 = direita
//2 = esquerda
TX_THREAD th_elevador[3];

TX_THREAD th_distribuidor_msg;



//fila para os elevadores
//0 = central
//1 = direita
//2 = esquerda
TX_QUEUE fila_elevador[3];

TX_QUEUE fila_uart;


TX_MUTEX uart_mutex;
/*
//flags não utilizadas
TX_EVENT_FLAG_GROUP flag_goup;
#define FLAG_INICIADO 0x00000001
#define FLAG_REQUEST_C 0x00000002
#define FLAG_REQUEST_D 0x00000004
#define FLAG_REQUEST_E 0x00000008
*/

#define INICIALIZACAO 'z'

//tratamento da interrupção 
void uart0_ISR(void){
  ULONG c = (ULONG)UARTCharGetNonBlocking(UART0_BASE);
  tx_queue_send(&fila_uart,&c,TX_NO_WAIT);
  UARTIntClear(UART0_BASE,UARTIntStatus(UART0_BASE,true));
}

//inicialização da uart
//a uart foi inicializada para ter interrupção após o recebimento de cada byte
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
  //UARTFIFOEnable(UART0_BASE);
  
  //habilita interrupção para recebimento e temporização
  //UARTFIFOLevelSet(UART0_BASE,UART_FIFO_TX7_8,UART_FIFO_RX4_8);//4/8 ou 8/16
  UARTIntEnable(UART0_BASE,UART_INT_RX|UART_INT_RT);
  UARTIntRegister(UART0_BASE,uart0_ISR);

}




//numero_elevador recebe e, d ou c
// e retorna o numero relativo do elevador usado em filas, threads etc
//char_elevador faz o contrario
//0 = central
//1 = direita
//2 = esquerda
ULONG numero_elevador(ULONG c){
  return c - 99;
}

ULONG char_elevador(ULONG n){
  return n + 99;
}


//verifica se o caractere representa o fim de mensagem
bool end_of_msg(ULONG c){
  return (c == '\n' || c == '\r');
}

//retorna o numero do andar dado um char
ULONG numero_andar(ULONG c){
  return c - 'a';
}

//retorna a letra do andar dado um número
ULONG letra_andar(ULONG c){
  return c + 'a';
}

//transforma um número que pode ser de 0 a 15, inicialmente codificado em ASCII,
ULONG char_to_numero_andar(ULONG *c,ULONG size){
  
  if(size == 2){
    return *(c) - '0';
  }
  return (((*c)-'0')*10 + *(c+1)-'0');
}

//receive_msg
//recebe mensagem de uma fila queue, a guardando em msg, que tem tamanho máximo
//max_length_msg. O fim da mensagem é dado pelo recebimento de um caractere definido
//em função end_of_msg.
//retorna o tamanho da mensagem + 1
ULONG receive_msg(TX_QUEUE *queue,ULONG *msg,ULONG max_length_msg){
  ULONG i = 1;
  ULONG aux;
  //recebe qualquer resquício de fim de msg até receber o primeiro caractere de msg
  do{
    tx_queue_receive(queue,&aux,TX_WAIT_FOREVER);
  }while(end_of_msg(aux));
  *msg = aux;
  //recebe o resto da msg
  if(max_length_msg!=1){
    do{
      if(TX_SUCCESS==tx_queue_receive(queue,&aux,TX_WAIT_FOREVER)){
        *(msg + i) = aux;
        i++;
      }
    }while(!end_of_msg(aux)&&(i<max_length_msg));
  }
  return i;
}

//define_elevador
//define para qual elevador devem ser enviada a mensagem
//0 = central
//1 = direita
//2 = esquerda
//3 = todos (caso de inicialização)
//255 = erro
ULONG define_elevador(ULONG *msg, ULONG size_msg){
  //initialized
  //uint8_t c;
  if(size_msg == 12){
    //set flag to initialize system
    return TODOS;
  }
  ULONG c = numero_elevador(*msg);
  if(c>2){
    if(size_msg == 6){//posição em mm
      //verificar qual elevador requisitou (trabalharia com as flags)
      //por enquanto não trabalharei com isso
      c = 255;
    }else{
      c = 255;
    }
  }
  return c;
}

//envia uma mensagem msg pela fila queue até enviar o caractere responsavel pelo
//fim da mensagem dado pela função end_of_msg
void send_msg(TX_QUEUE *queue,ULONG *msg){
  ULONG i = 0;
  ULONG aux;
  do{
    aux = *(msg + i);
    if(TX_SUCCESS==tx_queue_send(queue,(void*)&aux,TX_WAIT_FOREVER)){
      i++;
    }
  }while(!end_of_msg(aux));
}

//por enquanto o distribuidor de mensagens trabalhará somente com mensagens que contenham
void distribuidor_de_mensagens(ULONG input){
  ULONG local_buffer[14];
  while(1){
    ULONG size_msg = receive_msg(&fila_uart,local_buffer,13);
    ULONG e = define_elevador(local_buffer,size_msg);
    
    if(e != 255){
      if(e == TODOS){
        local_buffer[0] = 'z';
        local_buffer[1] = '\n';
        
        send_msg(&fila_elevador[0],local_buffer);
        send_msg(&fila_elevador[1],local_buffer);
        send_msg(&fila_elevador[2],local_buffer);
      }else{
        uint8_t i;
        if(numero_elevador(local_buffer[0])<3){//mensagem contém o nome do elevador
          i = 1;
        }else{//mensagem nao contem o nome do elevador ( requisição de posicao)
          i = 0;
        }
        send_msg(&fila_elevador[e],local_buffer + i);
      }
    }
  }
}

//envia uma mensagem pela uart
//função thread-safe porque adquire e libera o mutex entre cada chamada
void send_msg_uart0(ULONG *msg){
  tx_mutex_get(&uart_mutex,TX_WAIT_FOREVER);
  ULONG i = 0;
  char aux;
  do{
    aux = (char) *(msg + i);
//    if(UARTCharPutNonBlocking(UART0_BASE,aux)){
//      i++;
//    }
    UARTCharPut(UART0_BASE,aux);
    i++;

  }while(!end_of_msg(aux));
  tx_mutex_put(&uart_mutex);
}

//definição dos tipos de mensagem
#define TIPO_INICIALIZACAO 0
#define TIPO_CHEGADA_ANDAR 1
#define TIPO_PORTA_ABERTA 2
#define TIPO_PORTA_FECHADA 3
#define TIPO_BOTAO_INTERNO 4
#define TIPO_BOTAO_EXTERNO 5

//define qual o tipo da mensagem
ULONG define_tipo_msg(ULONG *msg, ULONG size_msg){
  ULONG c = *msg;
  if(size_msg < 4 && c <= '9'){
    return TIPO_CHEGADA_ANDAR;
  }
  if(c == 'I'){
    return TIPO_BOTAO_INTERNO;
  }
  if(c == 'E'){
    return TIPO_BOTAO_EXTERNO;
  }
  if(c == 'A'){
    return TIPO_PORTA_ABERTA;
  }
  if(c == 'F'){
    return TIPO_PORTA_FECHADA;
  }
  if(c == 'z'){
    return TIPO_INICIALIZACAO;
  }
  return 0xFFFFFFFF;
}

//thread do elevador
//input é o número do elevador correspondente como já definido
void elevador(ULONG input){
  char nome = input;
  ULONG msg_rec_buffer[7];
  ULONG msg_send_buffer[5];
  bool lista_paradas[16];
  
  ULONG andar_atual = 0;
  ULONG ultima_direcao = 's';
  
  bool porta_aberta = 0;
  ULONG i,n;
  for(i = 0;i<16;i++){
    lista_paradas[i] = false;
  }
  
  msg_send_buffer[0] = char_elevador(nome);

  
  while(1){

    ULONG size_msg = receive_msg(&fila_elevador[nome],msg_rec_buffer,6);
    ULONG tipo_msg = define_tipo_msg(msg_rec_buffer,size_msg);

    //define o tipo de mensagem e define a lógica para enviar mensagem
    switch(tipo_msg){
    case TIPO_INICIALIZACAO://formato da mensagem a ser enviada <elevador>r\r
      
      msg_send_buffer[1] = 'r';
      msg_send_buffer[2] = '\r';
      send_msg_uart0(msg_send_buffer);

/*      for(i = 0;i<16;i++){
        lista_paradas[i] = false;
      }
      ultima_direcao = 's';
      andar_atual = 0;
*/      
      break;
      
    case TIPO_CHEGADA_ANDAR:
      n = char_to_numero_andar(msg_rec_buffer,size_msg);
      andar_atual = n;
      
      //caso andar esteja na lista de paradas, para, caso contrário faz nada
      if(lista_paradas[n]){
        lista_paradas[n] = false;
        
        //envia mensagem para parar
        msg_send_buffer[1] = 'p';
        msg_send_buffer[2] = '\r';
        send_msg_uart0(msg_send_buffer);
        
        //envia mensagem para abrir a porta
        msg_send_buffer[1] = 'a';
        send_msg_uart0(msg_send_buffer);
        
        //envia mensagem para desligar a luz do botao
        msg_send_buffer[1] = 'D';
        msg_send_buffer[2] = letra_andar(n);
        msg_send_buffer[3] = '\r';
        send_msg_uart0(msg_send_buffer);
      }
      break;
      
      //
    case TIPO_PORTA_ABERTA:
      porta_aberta = true;
      
      //espera um tempo de porta aberta
      tx_thread_sleep(TICKS_PORTA_ABERTA);
      
      //verifica se há andar destino
      //uma outra forma de fazer isso é com um contador (mais performance)
      for(i = 0;i<16;i++){
        if(lista_paradas[i]){
          break;
        }
      }
      
      //se há andar destino
      if(i<16){
        msg_send_buffer[1] = 'f';
        msg_send_buffer[2] = '\r';
        send_msg_uart0(msg_send_buffer);
      }
      
      break;
      
      //pela lógica colocada, se a porta é fechada é porque é necessário ir a 
      //algum andar
    case TIPO_PORTA_FECHADA:
      porta_aberta = false;
      ULONG ns,nd;
      ns = andar_atual + 1;
      
      //implementar uma lista duplamente encadeada melhoraria a performance do 
      //trecho abaixo (e muito)
      
      //verifica andares para cima
      while(ns<16){
        if(lista_paradas[ns]){
          break;
        }
        ns++;
      }
      nd = andar_atual - 1;
      
      //verifica andares para baixo
      while(nd!=0xFFFFFFFF){
        if(lista_paradas[nd]){
          break;
        }
        nd--;
      }
      
      //define direção
      if(ns==16){
        ultima_direcao = 'd';
      }else if(nd == 0xFFFFFFFF){
        ultima_direcao = 's';
      }
        

      msg_send_buffer[1] = ultima_direcao;
      msg_send_buffer[2] = '\r';
      send_msg_uart0(msg_send_buffer);
      
      
      break;
      
      
    case TIPO_BOTAO_INTERNO:
      
      n = numero_andar(*(msg_rec_buffer+1));
      
      if(n==andar_atual)        break;
      
      
      msg_send_buffer[1] = 'L';
      msg_send_buffer[2] = msg_rec_buffer[1];
      msg_send_buffer[3] = '\r';
      send_msg_uart0(msg_send_buffer);
      
      
      
      if(!lista_paradas[n]){
        lista_paradas[n] = true;
        if(porta_aberta==true){
          msg_send_buffer[1] = 'f';
          msg_send_buffer[2] = '\r';
          send_msg_uart0(msg_send_buffer);
        }
      }
      
      
      
      
      
      break;
    case TIPO_BOTAO_EXTERNO:
      
      n = char_to_numero_andar((msg_rec_buffer+1),size_msg);
      
      if(n==andar_atual)        break;
            
      
      
      if(!lista_paradas[n]){
        lista_paradas[n] = true;
        if(porta_aberta==true){
          msg_send_buffer[1] = 'f';
          msg_send_buffer[2] = '\r';
          send_msg_uart0(msg_send_buffer);
        }
      }
      
      
      break;
    }
  }
  
}

//definição de constantes utilizadas na criação dos elementos como threads e 
//filas em tx_application_define

#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
UCHAR                   byte_pool_memory[DEMO_BYTE_POOL_SIZE];

//tempo dado a cada thread (500 ms)
#define thread_slice 50

void    tx_application_define(void *first_unused_memory){
  
CHAR    *pointer = TX_NULL;


#ifdef TX_ENABLE_EVENT_TRACE
    tx_trace_enable(trace_buffer, sizeof(trace_buffer), 32);
#endif

    tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory, DEMO_BYTE_POOL_SIZE);



    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&th_elevador[CENTRAL], "elevador central", elevador, CENTRAL,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, /*TX_NO_TIME_SLICE*/thread_slice, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&th_elevador[DIREITA], "elevador direita", elevador, DIREITA,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, /*TX_NO_TIME_SLICE*/thread_slice, TX_AUTO_START);
    
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&th_elevador[ESQUERDA], "elevador esquerda", elevador, ESQUERDA,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, /*TX_NO_TIME_SLICE*/thread_slice, TX_AUTO_START);
    

    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&th_distribuidor_msg, "thread 1", distribuidor_de_mensagens, 1,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, thread_slice, TX_AUTO_START);

    //estruturas
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&fila_elevador[0], "fila elevador 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&fila_elevador[1], "fila elevador 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));
    
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&fila_elevador[2], "fila elevador 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));

    
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&fila_uart, "fila uart", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));

    tx_mutex_create(&uart_mutex,"uart mutex",TX_NO_INHERIT);
    
    //necessario que a inicialização da uart seja feita aqui, porque ela se utiliza da fila
    uart0_init();
}

int main()
{
  tx_kernel_enter();
}
