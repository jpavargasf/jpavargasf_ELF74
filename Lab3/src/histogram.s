        PUBLIC EightBitHistogram
        
        SECTION .text : CODE(2)
        THUMB
;EigthBitHistogram:     calcula o histograma de uma imagem definida em níveis de 
;                       cinza e retorna o tamanho da imagem
;Entradas:
;       R0:largura da imagem (half-word)
;       R1:altura da imagem (half-word)                            
;       R2:ponteiro para a imagem (byte-image)
;       R3:ponteiro para o histograma a ser preenchido (half-word histogram)
;Retorno:
;       R0:tamanho da imagem (half-word) ou 0 se for maior que 64kB
EightBitHistogram
        ;PUSH {R4,LR}
        MOV R12, #65356
        MUL R0, R0, R1
        CMP R0, R12
        BLO EightBitHistogram_c
        MOV R0, #0
        B EigthBitHistogram_end
        
EightBitHistogram_c        
        PUSH {R4}
                              
        MOV R12, #0
        MOV R1, R3
EigthBitHistogram_reset                 ;zera o vetor histograma
        STR R12, [R1], #2
        SUB R4, R1, R3
        CMP R4, #512
        BNE EigthBitHistogram_reset
        
EigthBitHistogram_hist                  ;calcula o histograma
        LDRB R1, [R2, R12]
        LDRH R4, [R3, R1, LSL #1]
        ADD R4, #1
        STRH R4, [R3, R1, LSL #1]
        ADD R12, #1
        CMP R12, R0
        BNE EigthBitHistogram_hist
        
        POP {R4}
        
EigthBitHistogram_end
        ;POP {R4,PC}
        BX LR
        
        
        END