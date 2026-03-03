
#  Concorrência e Sincronização de Processos

## Sobre o projeto  

O Trabalho Prático 2 aprofunda os conceitos de concorrência e sincronização em Sistemas Operacionais. O projeto explora problemas clássicos envolvendo múltiplos processos ou threads acessando recursos compartilhados.

O objetivo é implementar mecanismos de controle que garantam integridade dos dados e evitem condições de corrida.

## Objetivos  

- Trabalhar com execução concorrente  
- Implementar mecanismos de sincronização  
- Evitar condições de corrida  
- Garantir exclusão mútua  

## Funcionalidades Implementadas  

- Criação de múltiplos processos ou threads  
- Uso de variáveis compartilhadas  
- Implementação de mecanismos de sincronização  
- Simulação de cenários concorrentes  
- Análise de comportamento sob concorrência  

## Conceitos Trabalhados  

- Threads  
- Concorrência  
- Exclusão mútua  
- Seção crítica  
- Semáforos  
- Mutex  
- Deadlock  

## Tecnologias Utilizadas  

- Linguagem C  
- Biblioteca pthread (quando aplicável)  
- GCC  
- Ambiente Linux  

## Como executar  

### Pré-requisitos  

- GCC instalado  
- Linux ou WSL  

### Compilação  

```bash
gcc *.c -lpthread -o tp2  
### Execução  
./tp2  

