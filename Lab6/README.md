<h3>Comentários gerais</h3>
<ul>
    <li>Tempo de execução de cada loop em torno de 1,5 us
        <ul>
            <li>Calculado cronometrando milhões de ciclosr</li>
        </ul>
    </li>
    <li>Timer tick por segundo aproximadamente como 460
        <ul>
            <li>
                Visto da mesma maneira que o tempo de execução de cada loop
            </li>
            <li>A definição de TX_TIMER_TICKS_PER_SECOND como 100 em "tx_api.h", que o valor default não se aplica
                <ul>
                    <li>Não sei porque.</li>
                </ul>
            </li>
        </ul>
    </li>
    <li>O modo de escalonamento pode ser escolhido alterando o retorno da função modo</li>
    <li>Na verdade a thread recebe 4 argumentos, devido à reutilização do código para os 5 modos
        <ul>
            <li>Os dois primeiros são como os explicitados</li>
            <li>O terceiro é o tempo em que a thread ficará suspensa</li>
            <li>O quarto é o job que a thread fará, que pode ser o normal(itens 'a', 'b' e 'c') ou o com mutex ('d' 'e')</li>
        </ul>
    </li>
</ul>


<h3>a) Escalonamento por time-slice de 50 ms. Todas as tarefas com mesma prioridade.</h3>
<ul>
    <li>Contanto que as threads estejam prontas, elas alternarão entre si a cada 50 ms, ou se apenas uma estiver pronta, tal executará sem interrupções</li>
</ul>

<h3>b) Escalonamento sem time-slice e sem preempção. Prioridades estabelecidas no passo 3. A preempção pode ser evitada com o “ 
preemption threshold” do ThreadX.</h3>
<ul>
    <li>As threads não são interrompidas por conta desabilitação da preempção</li>
    <li>Threads de maior prioridade da que está sendo executada não executam assim que prontas. Apenas depois que a thread que está executando fornecer por vontade própria, através de tx_sleep, o processador.</li>
    <li>Na maior parte do tempo as threads tem período constante, que só pode aumentar caso outra thread esteja em execução</li>
</ul>

<h3>c) Escalonamento preemptivo por prioridade.</h3>
<ul>
    <li>O período de execução da Thread 1 é sempre o mesmo de 1s, diferente do período das threads 2 e 3.
    <ul>
        <li>A thread 2 pode ser interrompida apenas pela 1 e a thread 3 pela 2 e 1.</li>
        <li>Thread 2 pode ter um tempo máximo desde o início até seu término de 500 ms + 300 ms. Já a Thread 3 800 ms + 500 ms + 300 ms + 300 ms</li>
    </ul>
</ul>

<h3>d) Implemente um Mutex compartilhado entre T1 e T3. No início de cada job estas tarefas devem solicitar este mutex e liberá-lo no 
final. Use mutex sem herança de prioridade. Observe o efeito na temporização das tarefas.</h3>
<ul>
    <li>Por conta do Mutex, T1 não pode dar preempção em T3 e vice-versa</li>
    <li>Caso T3 esteja executando e T2 mude de suspenso para pronto, T2 assumirá o processador</li>
    <li>Período de T2 pode alterar devido a T1</li>
    <li>Período de T3 pode alterar devido a T2 e se T1 estiver com o mutex</li>
    <li>Período de T1 pode alterar devido a T3 estar com o mutex</li>
<ul>

<h3>e) Idem acima, mas com herança de prioridade.</h3>
<ul>
    <li>Por conta do Mutex, T1 não pode dar preempção em T3 e vice-versa</li>
    <li>Caso T3 esteja executando e T2 mude de suspenso para pronto, T2 não assumirá o processador devido à herança de prioridade no mutex</li>
    <li>Período de T2 pode alterar devido a T1 e quando T3 estiver com mutex</li>
    <li>Período de T3 pode alterar devido a T2 e se T1 estiver com o mutex</li>
    <li>Período de T1 pode alterar devido a T3 estar com o mutex</li>
<ul>