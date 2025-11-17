#include <Servo.h>

// ================= VARIÁVEIS DE HARDWARE =================
Servo motorElevador;
Servo motorPorta;
const int sensorPorta = A0;
const int ledPorta = 8;
const int botaoTerreo = 4;
const int botaoPrimeiroAndar = 5;
const int botaoSegundoAndar = 6;
const int sinalizador = 3;

// ================= VARIÁVEIS DE ESTADO E FLUXO =================
int listaAndares[3];
int andarCount = 0;
int andarAtual = 0;
int destinoAtual = -1;
bool portaAberta = false;

// ================= VARIÁVEIS DE TEMPO (MILLIS) =================
unsigned long tempoInicioMovimento = 0;
unsigned long tempoMovimentoAtual = 0;

unsigned long tempoPortaAberta = 0;
const unsigned long duracaoPortaAberta = 5500;

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

// ================= TEMPOS DE VIAGEM ENTRE ANDARES =================
unsigned long tempoEntreAndares(int origem, int destino) {
  if (origem == 0 && destino == 1) {
    Serial.println("0 pro 1");
    return 2950; 
  }
  
  if (origem == 1 && destino == 2) {
    Serial.println("1 pro 2");
    return 2680;
  }

  if (origem == 2 && destino == 1) {
    Serial.println("2 pro 1");
    return 2800;
  }
  if (origem == 1 && destino == 0) {
    Serial.println("1 pro 0");
    return 3100;
  }

  Serial.println("Fallback.");
  return 1000; // fallback
}

// ================= SETUP =================
void setup() {
  motorElevador.attach(9);
  motorPorta.attach(10);
  pinMode(ledPorta, OUTPUT);
  pinMode(botaoTerreo, INPUT_PULLUP);
  pinMode(botaoPrimeiroAndar, INPUT_PULLUP);
  pinMode(botaoSegundoAndar, INPUT_PULLUP);
  pinMode(sinalizador, OUTPUT);

  Serial.begin(9600);
  Serial.println("Sistema de Elevador Iniciado");

  motorElevador.write(95);  // motor parado
  motorPorta.write(0);      // porta fechada
}

// ================= LOOP PRINCIPAL =================
void loop() {
  if (digitalRead(botaoTerreo) == LOW) adicionarAndar(0);
  if (digitalRead(botaoPrimeiroAndar) == LOW) adicionarAndar(1);
  if (digitalRead(botaoSegundoAndar) == LOW) adicionarAndar(2);

  delay(300); // debounce

  atualizarElevador();
}

// ================= FUNÇÕES AUXILIARES =================
void adicionarAndar(int novoAndar) {
  for (int i = 0; i < andarCount; i++)
    if (listaAndares[i] == novoAndar) return;

  if (andarCount < 3) {
    listaAndares[andarCount++] = novoAndar;
    Serial.print("Andar ");
    Serial.print(novoAndar);
    Serial.println(" adicionado à fila.");
  }
}

// ================= LÓGICA DE PRIORIZAÇÃO DE DESTINO =================
int encontrarProximoDestino() {
  if (andarCount == 0) return -1;

  int melhorIndex = 0;
  int menorDist = abs(listaAndares[0] - andarAtual);

  for (int i = 1; i < andarCount; i++) {
    int d = abs(listaAndares[i] - andarAtual);
    if (d < menorDist) {
      melhorIndex = i;
      menorDist = d;
    }
  }

  return melhorIndex;
}

// ================= LÓGICA PRINCIPAL =================
void atualizarElevador() {
  unsigned long agora = millis();

  if (destinoAtual == -1 && andarCount > 0) {
    int index = encontrarProximoDestino();
    destinoAtual = listaAndares[index];

    for (int i = index; i < andarCount - 1; i++)
      listaAndares[i] = listaAndares[i + 1];

    andarCount--;

    Serial.print("Novo destino: ");
    Serial.println(destinoAtual);
  }

  if (destinoAtual == -1 && estadoElevador == PARADO) return;

  switch (estadoElevador) {

    case PARADO:
      if (!portaAberta && destinoAtual != -1) {
        if (destinoAtual > andarAtual) estadoElevador = SUBINDO;
        else if (destinoAtual < andarAtual) estadoElevador = DESCENDO;
        else estadoElevador = ABRINDO_PORTA;
      }
      break;

    case SUBINDO:
      if (tempoInicioMovimento == 0) {
        motorElevador.write(110);

        tempoMovimentoAtual = tempoEntreAndares(andarAtual, andarAtual + 1);
        tempoInicioMovimento = agora;
      }

      if (agora - tempoInicioMovimento >= tempoMovimentoAtual) {
        motorElevador.write(95);
        andarAtual++;
        tempoInicioMovimento = 0;

        Serial.print("Subiu para andar ");
        Serial.println(andarAtual);

        if (andarAtual == destinoAtual)
          estadoElevador = ABRINDO_PORTA;
      }
      break;

    case DESCENDO:
      if (tempoInicioMovimento == 0) {
        motorElevador.write(85);

        tempoMovimentoAtual = tempoEntreAndares(andarAtual, andarAtual - 1);
        tempoInicioMovimento = agora;
      }

      if (agora - tempoInicioMovimento >= tempoMovimentoAtual) {
        motorElevador.write(95);
        andarAtual--;
        tempoInicioMovimento = 0;

        Serial.print("Desceu para andar ");
        Serial.println(andarAtual);

        if (andarAtual == destinoAtual)
          estadoElevador = ABRINDO_PORTA;
      }
      break;

    case ABRINDO_PORTA:
      tone(sinalizador, 1000, 800);
      abrirPorta();
      tempoPortaAberta = agora;
      estadoElevador = PORTA_ABERTA;
      break;

    case PORTA_ABERTA:
      if (agora - tempoPortaAberta >= duracaoPortaAberta)
        estadoElevador = FECHANDO_PORTA;
      break;

    case FECHANDO_PORTA:
      if (analogRead(sensorPorta) < 500) {
        Serial.println("Obstáculo detectado! Mantendo porta aberta.");
        estadoElevador = PORTA_ABERTA;
        tempoPortaAberta = agora;
      } else {
        fecharPorta();
        destinoAtual = -1;
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
