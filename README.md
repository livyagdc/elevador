# 🛗 Simulador de Elevador – Arduino

Este projeto implementa um **simulador completo de elevador** utilizando Arduino. Ele inclui gerenciamento de fila de andares, máquina de estados, controle de portas e simulação do movimento entre andares. A interação ocorre pelo **Serial Monitor**.

---

## 📌 Funcionalidades

* Seleção de andares pelo usuário (0 a 9)
* Fila inteligente de destinos, evitando duplicatas
* Prioridade baseada na direção atual do elevador
* Máquina de estados completa (subindo, descendo, porta abrindo, etc.)
* Simulação de movimento com `millis()` (sem `delay` bloqueante)
* Controle de tempo da porta aberta

---

## 🚀 Como Usar

1. Carregue o código no Arduino.
2. Abra o **Serial Monitor** em 9600 baud.
3. Digite um número entre **0 e 9**.
4. Observe o elevador:

   * adicionando destinos à fila;
   * movendo-se entre andares;
   * abrindo e fechando portas;
   * atendendo múltiplas chamadas sequencialmente.

---

## 🧠 Máquina de Estados do Elevador

O elevador utiliza um sistema baseado no estado atual:

| Estado             | Função                        |
| ------------------ | ----------------------------- |
| **PARADO**         | Aguardando comandos           |
| **SUBINDO**        | Movendo em direção superior   |
| **DESCENDO**       | Movendo em direção inferior   |
| **ABRINDO_PORTA**  | Início da abertura da porta   |
| **PORTA_ABERTA**   | Porta aberta aguardando tempo |
| **FECHANDO_PORTA** | Finaliza fechamento           |

---

## 🎯 Gerenciamento da Fila de Destinos

* Fila com capacidade para **até 10 andares**.
* Não adiciona andares duplicados.
* Prioriza andares no caminho da direção atual.
* Caso contrário, escolhe o destino **mais próximo**.

---

## ⏱️ Simulação de Movimento

* Tempo entre andares: **1.5s**
* Porta fica aberta por **3s**
* Todo o tempo é controlado com `millis()` para não travar o loop
