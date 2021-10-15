#include <cstdlib>
#include <iostream>
#include <stdint.h>

#include "images.c"

//using namespace std;

extern "C" uint16_t EightBitHistogram(uint16_t width, uint16_t height, const uint8_t * p_image, uint16_t * p_histogram);

//printa o histograma
//Ela utiliza printf(...) ao invez de std::cout<<... porque ocorria um erro quando o argumento era um número
void printArray(uint16_t ui16Size, uint16_t *ui16Array){
  if(ui16Array==NULL)   return;
  uint16_t ui16Counter = 0;//contador de posicoes
  
  //auxiliares que servem para agrupar em um só printf elementos consecutivos iguais
  uint16_t ui16PosAnt = 0;
  //uint16_t ui16PosDep = 0;
  
  do{
    if(((ui16Counter+1)<ui16Size)&&(ui16Array[ui16Counter]==ui16Array[ui16Counter+1])){
      ui16PosAnt = ui16Counter;
      do{
        ui16Counter++;
      }while(((ui16Counter)<ui16Size)&&(ui16Array[ui16PosAnt]==ui16Array[ui16Counter]));
      printf("[%d:%d]:%d\n",ui16PosAnt,ui16Counter-1,ui16Array[ui16PosAnt]);
    }else{
      printf("%d:%d\n",ui16Counter,ui16Array[ui16Counter]);
      ui16Counter++;
    }
  }while(ui16Counter<ui16Size);
}

//compara se dois arays são iguais
uint8_t compareArray(uint16_t ui16Size, uint16_t *ui16Array0, uint16_t *ui16Array1){
  uint8_t ui8Result = 0;
  if(((ui16Array0==NULL)&&(ui16Array1!=NULL))||((ui16Array1==NULL)&&(ui16Array0!=NULL))){
    ui8Result = 1;
  }
  if(ui16Array0!=NULL&&ui16Array1!=NULL){
    uint16_t ui16Counter = 0;
    while((ui16Counter<ui16Size)&&(ui16Array0[ui16Counter]==ui16Array1[ui16Counter])){
      ui16Counter++;
    }
    if(ui16Counter!=ui16Size){
      ui8Result = 1;
    }
  }
  return ui8Result;
}

//função em c++ equivalente à EightBitHistogram(...)
uint16_t cppEightBitHistogram(uint16_t width, uint16_t height, const uint8_t * p_image, uint16_t * p_histogram){
  uint16_t ui16Size = width*height;
  if((uint32_t)ui16Size==((uint32_t)(width*height))){
    //se ui16Size for menor que width*heigth, indica que ha overflow e que 
    //o tamanho da imagem é maior que 65535
    uint16_t ui16Counter = 0;
    do{
      p_histogram[ui16Counter] = 0;
    }while(++ui16Counter<256);
    ui16Counter = 0;
    do{
      p_histogram[p_image[ui16Counter]]++;
    }while(++ui16Counter<ui16Size);
  }
  return ui16Size;
}

//funcao principal, que compara as duas 
void principal(uint16_t width,uint16_t heigth,const uint8_t * p_start_image,uint16_t * histogram0,uint16_t * histogram1){
  uint16_t ui16Size0,ui16Size1;  
  ui16Size0 = EightBitHistogram(width,heigth, p_start_image, histogram0);
  ui16Size1 = cppEightBitHistogram(width,heigth, p_start_image, histogram1);
  if((compareArray(256,histogram0,histogram1)==1)||(ui16Size0!=ui16Size1)){
    std::cout<<"Erro : Histograma Assembly e C++ diferentes!"<<std::endl;
  }
  else{
    printArray(256,histogram0);
  }
}

int main()
{
  uint16_t *histogram,*cppHistogram;
  
  histogram = (uint16_t*)malloc(256*sizeof(uint16_t));
  cppHistogram = (uint16_t*)malloc(256*sizeof(uint16_t));
  
  std::cout<<"Imagem 0 (modificada)"<<std::endl;
  principal(width0,height0, p_start_image0, histogram, cppHistogram);
  std::cout<<"Imagem 1"<<std::endl;
  principal(width1,height1, p_start_image1, histogram, cppHistogram);
  
  return 0;
}
