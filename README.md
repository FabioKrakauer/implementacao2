# Implementação 2 - Gerenciamento de Memoria


## Objetivo

Este exercício tem como objetivo fazer um gerenciamento de memoria virtual, com tratamento de endereços lógicos, para endereços fisicos e algoritmos de substituição da TLB e da memoria fisica com FIFO e LRU.

## Como executar

Para executar o programa basta rodar os seguintes comandos:
**"make"**: Este comando ira compilar o arquivo 'frk.c' para vm

**./vm addresses.txt fifo fifo**: Para executar o script e receber as respostas no arquivo correct.txt. Os últimos 3 argumentos podem ser dinâmicos e aplicados da forma necessária. Como por exemplo ./vm me_da_dez.txt lru fifo.

**"make clean"**: Para limpar o arquivo vm e poder compilar novamente com o primeiro comando!