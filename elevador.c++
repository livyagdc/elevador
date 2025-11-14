#include <Servo.h>

// ================= VARIÁVEIS DE HARDWARE =================
Servo motorElevador;
Servo motorPorta;
const int sensorPorta = A0;
const int ledPorta = 8;
const int botaoTerreo = 6;
const int botaoPrimeiroAndar = 5;
const int botaoSegundoAndar = 4;

// ================= VARIÁVEIS DE ESTADO E FLUXO =================
int listaAndares[3];
int andarCount = 0;
int andarAtual = 0;
int destinoAtual = -1;
bool portaAberta = false;

// ================= VARIÁVEIS DE TEMPO (MILLIS) =================
unsigned long ultimoTempo = 0;
const unsigned long intervaloMovimento = 1500;  // Tempo para simular a viagem de um andar (1.5s)
unsigned long tempoInicioMovimento = 0;         // para controlar o tempo que o motor está ligado

unsigned long tempoPortaAberta = 0;
const unsigned long duracaoPortaAberta = 3000;  // Tempo que a porta fica aberta (3s)

// ================= ESTADOS DO ELEVADOR =================
enum EstadoElevador {
  PARADO,
  SUBINDO,
  DESCENDO,
  ABRINDO_PORTA,
  PORTA_ABERTA,
  FECHANDO_PORTA
};

EstadoElevador estadoElevador = PARADO;

// ================= SETUP =================
void setup() {
  motorElevador.attach(9);
  motorPorta.attach(10);
  pinMode(ledPorta, OUTPUT);
  pinMode(botaoTerreo, INPUT_PULLUP);
  pinMode(botaoPrimeiroAndar, INPUT_PULLUP);
  pinMode(botaoSegundoAndar, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Sistema de Elevador Iniciado");

  // Garantir que o motor comece parado
  motorElevador.write(95);
  // Garantir que a porta comece fechada
  motorPorta.write(0);
}

// ================= LOOP PRINCIPAL =================
void loop() {
  // --- Detectar botões de chamada ---
  if (digitalRead(botaoTerreo) == LOW) {
    adicionarAndar(0);
    Serial.print("Andar ");
    Serial.print(0);
    Serial.println(" adicionado à fila.");
  }

  if (digitalRead(botaoPrimeiroAndar) == LOW) {
    adicionarAndar(1);
    Serial.print("Andar ");
    Serial.print(1);
    Serial.println(" adicionado à fila.");
  }

  if (digitalRead(botaoSegundoAndar) == LOW) {
    adicionarAndar(2);
    Serial.print("Andar ");
    Serial.print(2);
    Serial.println(" adicionado à fila.");
  }

  delay(300);  // Debounce simples para o botão físico

  // --- Atualizar o movimento e estado do elevador ---
  atualizarElevador();
}

// ================= FUNÇÕES AUXILIARES =================
void adicionarAndar(int novoAndar) {
  // Não adiciona se já estiver na lista
  for (int i = 0; i < andarCount; i++) {
    if (listaAndares[i] == novoAndar)
      return;
  }

  // Adiciona se houver espaço
  if (andarCount < 10) {
    listaAndares[andarCount++] = novoAndar;
  } else {
    Serial.println("ERRO: Fila de andares cheia!");
  }
}

// ================= LÓGICA DE PRIORIZAÇÃO DE DESTINO =================
int encontrarProximoDestino() {
  if (andarCount == 0) return -1;

  // A direção que o elevador está indo ou pretende ir
  int direcao = 0;
  if (estadoElevador == SUBINDO) direcao = 1;
  else if (estadoElevador == DESCENDO) direcao = -1;
  // Se PARADO, a direção será determinada no próximo passo

  int melhorIndex = -1;

  // 1. Prioridade: Encontrar o mais próximo que está no caminho atual
  if (direcao != 0) {
    for (int i = 0; i < andarCount; i++) {
      bool noCaminho = (direcao > 0 && listaAndares[i] > andarAtual) || (direcao < 0 && listaAndares[i] < andarAtual);

      if (noCaminho) {
        // Se for o primeiro que atende o critério OU se estiver mais perto
        if (melhorIndex == -1 || abs(listaAndares[i] - andarAtual) < abs(listaAndares[melhorIndex] - andarAtual)) {
          melhorIndex = i;
        }
      }
    }
    // Se encontramos um destino no caminho, retornamos ele
    if (melhorIndex != -1) {
      return melhorIndex;
    }
  }

  // 2. Se não há nada no caminho, ou se está PARADO, pegue o mais próximo de todos
  melhorIndex = 0;
  int menorDistancia = abs(listaAndares[0] - andarAtual);

  for (int i = 1; i < andarCount; i++) {
    int distancia = abs(listaAndares[i] - andarAtual);
    if (distancia < menorDistancia) {
      menorDistancia = distancia;
      melhorIndex = i;
    }
  }
  return melhorIndex;
}


// ================= LÓGICA PRINCIPAL =================
void atualizarElevador() {
  unsigned long agora = millis();

  // --- Lógica de Seleção de Destino (Executa apenas quando o elevador está livre) ---
  if (destinoAtual == -1 && andarCount > 0) {
    int proximoDestinoIndex = encontrarProximoDestino();

    if (proximoDestinoIndex != -1) {
      destinoAtual = listaAndares[proximoDestinoIndex];

      // Remove o destino escolhido da lista
      for (int i = proximoDestinoIndex; i < andarCount - 1; i++) {
        listaAndares[i] = listaAndares[i + 1];
      }
      andarCount--;

      Serial.print("Novo destino definido: Andar ");
      Serial.println(destinoAtual);
    }
  }

  if (destinoAtual == -1 && estadoElevador == PARADO) return;  // Nada pra fazer

  switch (estadoElevador) {
    case PARADO:
      if (!portaAberta && destinoAtual != -1) {
        if (destinoAtual > andarAtual) {
          estadoElevador = SUBINDO;
        } else if (destinoAtual < andarAtual) {
          estadoElevador = DESCENDO;
        } else {
          // Já está no destino
          estadoElevador = ABRINDO_PORTA;
        }
      }
      break;

    case SUBINDO:
      // Passo 1: Ligar o motor e registrar o tempo
      if (tempoInicioMovimento == 0) {
        motorElevador.write(100);  // Ligar motor para subir
        tempoInicioMovimento = agora;
      }

      // Passo 2: Esperar o tempo de simulação de movimento (1.5s)
      if (agora - tempoInicioMovimento >= intervaloMovimento) {
        // Passo 3: Parar o motor e atualizar o estado
        motorElevador.write(95);  // Parar motor
        andarAtual++;
        tempoInicioMovimento = 0;  // Resetar para o próximo movimento

        Serial.print("Subiu. Agora no andar ");
        Serial.println(andarAtual);

        if (andarAtual == destinoAtual) {
          estadoElevador = ABRINDO_PORTA;
        } else {
          // Se o próximo destino está na mesma direção, volta para SUBINDO
          if (destinoAtual > andarAtual) {
            estadoElevador = SUBINDO;
          } else {
            // Se atingiu o destino final
            estadoElevador = PARADO;
          }
        }
      }
      break;

    case DESCENDO:
      // Passo 1: Ligar o motor e registrar o tempo
      if (tempoInicioMovimento == 0) {
        motorElevador.write(90);  // Ligar motor para descer
        tempoInicioMovimento = agora;
      }

      // Passo 2: Esperar o tempo de simulação de movimento (1.5s)
      if (agora - tempoInicioMovimento >= intervaloMovimento) {
        // Passo 3: Parar o motor e atualizar o estado
        motorElevador.write(95);  // Parar motor
        andarAtual--;
        tempoInicioMovimento = 0;  // Resetar para o próximo movimento

        Serial.print("Desceu. Agora no andar ");
        Serial.println(andarAtual);

        if (andarAtual == destinoAtual) {
          estadoElevador = ABRINDO_PORTA;
        } else {
          // Se o próximo destino está na mesma direção, volta para DESCENDO
          if (destinoAtual < andarAtual) {
            estadoElevador = DESCENDO;
          } else {
            // Se atingiu o destino final
            estadoElevador = PARADO;
          }
        }
      }
      break;

    case ABRINDO_PORTA:
      abrirPorta();
      tempoPortaAberta = agora;  // Registra o momento em que abriu
      estadoElevador = PORTA_ABERTA;
      break;

    case PORTA_ABERTA:
      // Verifica o tempo que a porta está aberta
      if (agora - tempoPortaAberta >= duracaoPortaAberta) {
        estadoElevador = FECHANDO_PORTA;
      }
      break;

    case FECHANDO_PORTA:
      // Verifica o sensor de obstáculo
      if (analogRead(sensorPorta) < 50) {
        Serial.println("Atenção: Obstáculo na porta! Porta permanece aberta.");
        // Volta para o estado PORTA_ABERTA para esperar o tempo de novo
        estadoElevador = PORTA_ABERTA;
        tempoPortaAberta = agora;  // Reinicia o cronômetro de espera
      } else {
        fecharPorta();
        destinoAtual = -1;  // Libera o elevador para pegar o próximo destino
        estadoElevador = PARADO;
      }
      break;
  }
}

// ================= PORTA =================
void abrirPorta() {
  if (!portaAberta) {
    motorPorta.write(120);
    digitalWrite(ledPorta, HIGH);
    portaAberta = true;
    Serial.println("Porta aberta");
  }
}
void fecharPorta() {
  if (portaAberta) {
    motorPorta.write(0);
    digitalWrite(ledPorta, LOW);
    portaAberta = false;
    Serial.println("Porta fechada");
  }
}