// ================= VARIÁVEIS DE ESTADO E FLUXO =================
int listaAndares[10];
int andarCount = 0;
int andarAtual = 0;
int destinoAtual = -1;
bool portaAberta = false;

// ================= VARIÁVEIS DE TEMPO (MILLIS) =================
const unsigned long intervaloMovimento = 1500; // Tempo para simular a viagem de um andar (1.5s)
unsigned long tempoInicioMovimento = 0;        // Para controlar o tempo que o elevador está em movimento

unsigned long tempoPortaAberta = 0;
const unsigned long duracaoPortaAberta = 3000; // Tempo que a porta fica aberta (3s)

// ================= MÁQUINA DE ESTADOS =================
enum EstadoElevador
{
    PARADO,
    SUBINDO,
    DESCENDO,
    ABRINDO_PORTA,
    PORTA_ABERTA,
    FECHANDO_PORTA
};

EstadoElevador estadoElevador = PARADO;

// ================= SETUP =================
void setup()
{
    Serial.begin(9600);
    Serial.println("Sistema de Elevador Iniciado");
}

// ================= LOOP PRINCIPAL =================
void loop()
{
    Serial.println("Digite o andar desejado (0-9):");

    unsigned long inicioLeitura = millis();
    while (Serial.available() == 0 && (millis() - inicioLeitura < 5000)); // Timeout de 5s para leitura

    if (Serial.available() > 0)
    {
        int andarChamado = Serial.parseInt();
        if (andarChamado >= 0 && andarChamado <= 9)
        {
            adicionarAndar(andarChamado);
            Serial.print("Andar ");
            Serial.print(andarChamado);
            Serial.println(" adicionado à fila.");
        }
    }
    delay(200);

    atualizarElevador();
}

// ================= FUNÇÕES AUXILIARES =================
void adicionarAndar(int novoAndar)
{
    // Não adiciona se já estiver na lista
    for (int i = 0; i < andarCount; i++)
    {
        if (listaAndares[i] == novoAndar)
            return;
    }

    // Adiciona se houver espaço
    if (andarCount < 10)
    {
        listaAndares[andarCount++] = novoAndar;
    }
    else
    {
        Serial.println("ERRO: Fila de andares cheia!");
    }
}

// ================= LÓGICA DE PRIORIZAÇÃO DE DESTINO =================
int encontrarProximoDestino()
{
    if (andarCount == 0)
        return -1;

    // A direção que o elevador está indo ou pretende ir
    int direcao = 0;
    if (estadoElevador == SUBINDO)
        direcao = 1;
    else if (estadoElevador == DESCENDO)
        direcao = -1;
    // Se PARADO, a direção será determinada no próximo passo

    int melhorIndex = -1;

    //Prioridade: Encontrar o mais próximo que está no caminho atual
    if (direcao != 0)
    {
        for (int i = 0; i < andarCount; i++)
        {
            bool noCaminho = (direcao > 0 && listaAndares[i] > andarAtual) ||
                             (direcao < 0 && listaAndares[i] < andarAtual);

            if (noCaminho)
            {
                // Se for o primeiro que atende o critério OU se estiver mais perto
                if (melhorIndex == -1 || abs(listaAndares[i] - andarAtual) < abs(listaAndares[melhorIndex] - andarAtual))
                {
                    melhorIndex = i;
                }
            }
        }
        // Se encontramos um destino no caminho, retornamos ele
        if (melhorIndex != -1)
        {
            return melhorIndex;
        }
    }

    // 2. Se não há nada no caminho, ou se está PARADO, pegue o mais próximo de todos
    melhorIndex = 0;
    int menorDistancia = abs(listaAndares[0] - andarAtual);

    for (int i = 1; i < andarCount; i++)
    {
        int distancia = abs(listaAndares[i] - andarAtual);
        if (distancia < menorDistancia)
        {
            menorDistancia = distancia;
            melhorIndex = i;
        }
    }
    return melhorIndex;
}

// ================= LÓGICA PRINCIPAL DA MÁQUINA DE ESTADOS =================
void atualizarElevador()
{
    unsigned long agora = millis();

    // --- Lógica de Seleção de Destino (Executa apenas quando o elevador está livre) ---
    if (destinoAtual == -1 && andarCount > 0)
    {
        int proximoDestinoIndex = encontrarProximoDestino();

        if (proximoDestinoIndex != -1)
        {
            destinoAtual = listaAndares[proximoDestinoIndex];

            // Remove o destino escolhido da lista
            for (int i = proximoDestinoIndex; i < andarCount - 1; i++)
            {
                listaAndares[i] = listaAndares[i + 1];
            }
            andarCount--;

            Serial.print("Novo destino definido: Andar ");
            Serial.println(destinoAtual);
        }
    }

    if (destinoAtual == -1 && estadoElevador == PARADO)
        return; // Nada pra fazer

    // --- Estados do Elevador ---
    switch (estadoElevador)
    {
    case PARADO:
        if (!portaAberta && destinoAtual != -1)
        {
            // O tempo de movimento deve ser inicializado ao MUDAR para SUBINDO/DESCENDO
            if (destinoAtual > andarAtual)
            {
                estadoElevador = SUBINDO;
                tempoInicioMovimento = agora; // Inicia o temporizador do primeiro movimento
            }
            else if (destinoAtual < andarAtual)
            {
                estadoElevador = DESCENDO;
                tempoInicioMovimento = agora; // Inicia o temporizador do primeiro movimento
            }
            else
            {
                // Já está no destino
                estadoElevador = ABRINDO_PORTA;
            }
        }
        break;

    case SUBINDO:
        // Esperar o tempo de simulação de movimento (1.5s)
        if (agora - tempoInicioMovimento >= intervaloMovimento) {
            andarAtual++;
            tempoInicioMovimento = agora; // Resetar/Atualizar para o próximo movimento
            
            Serial.print("Subiu. Agora no andar ");
            Serial.println(andarAtual);

            if (andarAtual == destinoAtual) {
                estadoElevador = ABRINDO_PORTA;
            }
        }
        break;

    case DESCENDO:
        // Esperar o tempo de simulação de movimento (1.5s)
        if (agora - tempoInicioMovimento >= intervaloMovimento) {
            andarAtual--;
            tempoInicioMovimento = agora; // Resetar/Atualizar para o próximo movimento
            
            Serial.print("Desceu. Agora no andar ");
            Serial.println(andarAtual);

            if (andarAtual == destinoAtual) {
                estadoElevador = ABRINDO_PORTA;
            }
        }
        break;

    case ABRINDO_PORTA:
        abrirPorta(); // Agora inicializa tempoPortaAberta
        estadoElevador = PORTA_ABERTA;
        break;

    case PORTA_ABERTA:
        // Verifica o tempo que a porta está aberta
        if (agora - tempoPortaAberta >= duracaoPortaAberta)
        {
            estadoElevador = FECHANDO_PORTA;
        }
        break;

    case FECHANDO_PORTA:
        fecharPorta();
        destinoAtual = -1; // Libera o elevador para pegar o próximo destino
        estadoElevador = PARADO;
        break;
    }
}

// ================= PORTA =================
void abrirPorta()
{
    if (!portaAberta)
    {
        portaAberta = true;
        tempoPortaAberta = millis();
        Serial.println("Porta aberta");
    }
}
void fecharPorta()
{
    if (portaAberta)
    {
        portaAberta = false;
        Serial.println("Porta fechada");
    }
}