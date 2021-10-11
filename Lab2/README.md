<h3>
	6.O exemplo original do blinky usa um loop para gerar a temporização da piscada do LED.
	Porque a variável ui32Loop é declarada como volatile dentro de main() ?
</h3>
<p>
	Para o compilador não otimizar tal variável. Ou seja, como não há nenhuma ação sendo feita realmente
	pela variável e.g. está somente alterando os valores da variável e comparando com outro somente para
	o efeito de delay, em um for loop vazio.
</p>