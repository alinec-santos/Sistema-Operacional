# Sistemas Operacionais  
Trabalhos Práticos – TP1 e TP2

---

# TP1 – Gerenciamento de Processos e Escalonamento

## Sobre o projeto  

O Trabalho Prático 1 tem como objetivo aplicar conceitos fundamentais de gerenciamento de processos em Sistemas Operacionais. O projeto consiste na implementação e simulação de políticas de escalonamento de CPU, permitindo a análise do comportamento dos processos durante a execução.

O foco principal está na compreensão do ciclo de vida de um processo e na comparação de algoritmos de escalonamento.

## Objetivos  

- Compreender o ciclo de vida de um processo  
- Implementar algoritmos de escalonamento  
- Calcular métricas como tempo de espera e tempo de retorno  
- Analisar desempenho de diferentes estratégias  

## Funcionalidades Implementadas  

- Leitura de processos com tempo de chegada e tempo de execução  
- Simulação da execução na CPU  
- Implementação de algoritmos de escalonamento  
- Cálculo de métricas de desempenho  
- Exibição da ordem de execução  

## Algoritmos Trabalhados  

- FCFS (First Come, First Served)  
- SJF (Shortest Job First)  
- Round Robin  
- Escalonamento por Prioridade (quando aplicável)  

## Tecnologias Utilizadas  

- Linguagem C  
- GCC  
- Ambiente Linux  

## Como executar  

### Pré-requisitos  

- GCC instalado  
- Linux ou WSL  

### Compilação  

gcc *.c -o tp1  

### Execução  

./tp1  

---

# TP2 – Concorrência e Sincronização de Processos

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

gcc *.c -lpthread -o tp2  

### Execução  

./tp2  

---

## Estrutura do Repositório  

- Pasta TP1  
- Pasta TP2  
- README.md  

