#define SENSOR_BAIXA    2  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR OPTICO PECA BAIXA */
#define SENSOR_MEDIA    3  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR OPTICO PECA MEDIA */
#define SENSOR_ALTA     4  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR OPTICO PECA ALTA */
#define RAMPA_1         5  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR DE FIBRA OPTICA DA RAMPA 1 */
#define RAMPA_2         6  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR DE FIBRA OPTICA DA RAMPA 2 */
#define RAMPA_3         7  /* PINO USADO COMO ENTRADA - LEITURA DO SENSOR DE FIBRA OPTICA DA RAMPA 3 */
#define ESTEIRA         8  /* PINO USADO COMO ENTRADA -  */
#define BT_RESET        9  /* PINO USADO COMO ENTRADA -  */
#define BT_ESTEIRA      10 /* PINO USADO COMO ENTRADA -  */
#define SOLENOIDE_1_A   11 /* PINO USADO COMO SAIDA - VALVULA SOLENOIDE DUPLA AVANCO */
#define SOLENOIDE_1_R   12 /* PINO USADO COMO SAIDA - VALVULA SOLENOIDE DUPLA RECUO */
#define SOLENOIDE_2     13 /* PINO USADO COMO SAIDA - VALVULA SOLENOIDE SIMPLES */
#define SOLENOIDE_3     14 /* PINO USADO COMO SAIDA - VALVULA SOLENOIDE SIMPLES */
#define TIMEOUT         1000 /* NAO DEFINIDO */
/*  tipos de altura de peca. Possui um valor nulo para funcionar como 
    inicializacao */
typedef enum
{
    _NULL,
    BAIXA,
    MEDIA,
    ALTA
} altura_peca_t;

typedef enum
{
    WAIT_LOW,
    WAIT_MED,
    WAIT_HIGH,
    HANDLER
} sm_peca_t;

typedef struct
{
    altura_peca_t altura;
    uint32_t timer;/* variavel de tempo que deve ser utilizada para estimar o 
        posicionamento da peca na esteira e assim acionar os atuadores */
    bool valid;/* indica se a peca e valida ou nao, ou seja, se deve ser 
        descartada ou nao */
    uint8_t posicao;
} peca_t;

typedef struct
{
    uint8_t count_pecas;
    const uint8_t maximo;
    altura_peca_t altura_pecas[5];
    const peca_t padrao[5];
} dispenser_t;

typedef struct
{
    uint8_t count_pecas;/* indica a quantidade de pecas ja recebidas pelo 
        sistema */
    peca_t pecas[20];/* vetor contendo as informacoes de todas as pecas */
    uint8_t next_index;/* ponteiro para a proxima peca a ser guardada nos 
        dispenseres */
    uint8_t next_receive;/* ponteiro para a posicao do vetor onde deve ser 
        armazenada as informacoes de uma nova peca */
} info_pecas_t;

info_pecas_t pecas_recebidas = {0, {_NULL, 0, false, 0}, 0, 0};
dispenser_t dispenser_0 = {0, 3, {_NULL}, {MEDIA, BAIXA, MEDIA, _NULL, _NULL}};
dispenser_t dispenser_1 = {0, 3, {_NULL}, {BAIXA, MEDIA, BAIXA, _NULL, _NULL}};
dispenser_t dispenser_2 = {0, 4, {_NULL}, {ALTA, ALTA, ALTA, ALTA, _NULL}};
peca_t nova_peca = {_NULL, millis(), 0, 0};
sm_peca_t sm_peca = WAIT_LOW;
uint32_t timer_aux = 0;
void setup()
{
    // put your setup code here, to run once:
    pinMode(SENSOR_BAIXA, INPUT);
    pinMode(SENSOR_MEDIA, INPUT);
    pinMode(SENSOR_ALTA, INPUT);
    pinMode(RAMPA_1, INPUT);
    pinMode(RAMPA_2, INPUT);
    pinMode(RAMPA_3, INPUT);
    pinMode(BT_RESET, INPUT);

    pinMode(SOLENOIDE_1_A, OUTPUT);
    pinMode(SOLENOIDE_1_R, OUTPUT);
    pinMode(SOLENOIDE_2, OUTPUT);
    pinMode(SOLENOIDE_3, OUTPUT);
}

void loop()
{
    
    /*  Ao receber um nivel logico alto no sensor optico de peca baixa, 
        significa que uma nova peca etrou para processamento. A ideia geral e 
        aguardar a peca passar pelos 3 sensores para entao tomar algum tipo de 
        decisao. A maquina de estados fica presa no estado WAIT_LOW ate receber 
        um nivel logico em SENSOR_BAIXA significando que identificou uma peca 
        passando pelo sensor. Apos isto, passa para o proximo nivel e aguarda um
        sinal em  SENSOR_MEDIA; este sinal pode ser nunca recebido (peca baixa),
        portanto é calculado um timeout de espera, utilizando o a tempo atual do
        programa (millis()) e a variavel de tempo da peca (nova_peca.timer), o 
        valor deste timeout deve ser definido com base na velocidade da esteira,
        pode ser fixo ou variavel (determinado a partir da leitura do ADC0 por 
        exemplo); ainda não pensei nesta parte, vou definir como TIMEOUT apenas.
        Se nao receber um sinal em SENSOR_MEDIA ja e possivel saber que e uma 
        peca baixa, portanto pode-se pular diretamente para HANDLER para fazer a
        analise da peca e determinar se a peca deve ser salva (nova_peca.valid) 
        e em qual dispenser ela deve ser salva (nova_peca.posicao). 
    */
    switch (sm_peca)
    {
        case WAIT_LOW:
            if (1 == digitalRead(SENSOR_BAIXA))
            {
                /*  salva uma nova peca como baixa, o tempo atual e como 
                    invalida. Passa para o proximo estado de processamento.  */
                nova_peca = {BAIXA, millis(), false};
                sm_peca = WAIT_MED;
            }
        break;

        case WAIT_MED:
            if(1 == digitalRead(SENSOR_MEDIA))
            {
                /*  ao receber o sinal de SENSOR_MEDIA, classifica nova peca 
                como media */
                nova_peca.altura = MEDIA;
                sm_peca = WAIT_HIGH;
            }
            else
            {
                timer_aux = millis() - nova_peca.timer;
                /*  checa se o timeout estourou e passa para HANDLER */
                if(timer_aux > TIMEOUT)
                {
                    sm_peca = HANDLER;
                }
            }
        break;

        case WAIT_HIGH:
            if(1 == digitalRead(SENSOR_ALTA))
            {
                /*  ao receber o sinal de SENSOR_ALTA, classifica nova peca 
                como alta */
                nova_peca.altura = ALTA;
                sm_peca = HANDLER;
            }
            else
            {
                timer_aux = millis() - nova_peca.timer;
                /*  checa se o timeout estourou e passa para HANDLER */
                if(timer_aux > TIMEOUT)
                {
                    sm_peca = HANDLER;
                }
            }
        break;

        case HANDLER:
            /*  regras para as pecas serem validas:
                * Sempre que três peças da mesma categoria forem inseridas na 
                  esteira, a peça repetida deve ser descartada.
                * Sempre que a quantidade de peças ultrapassar 20 unidades, a 
                  esteira deve ser automaticamente desligada e somente 
                  reiniciara a operar após pressionar o botão liga/desliga.                
            */

            /*  primeiro checa se o limite total de pecas nao foi atingido */
            if (20 >= pecas_recebidas.count_pecas)
            {
                /*  parar esteira */
            }
            /*  checa se existem duas pecas anteriores para serem validadas, se 
                nao existir marca como valida
            */
            else if (2 < pecas_recebidas.count_pecas)
            {
                nova_peca.valid = true;
                pecas_recebidas.count_pecas++;
                /*  salva informacoes da peca atual no vetor de pecas recebidas 
                */
                pecas_recebidas.pecas[pecas_recebidas.next_receive].altura = \
                    nova_peca.altura;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].timer = \
                    nova_peca.timer;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].valid = \
                    nova_peca.valid;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].posicao = \
                    nova_peca.posicao;
            }
            /*  compara com as duas pecas anteriores, se as tres forem de 
                alturas iguais, descarta-se a nova peca recebida
            */
            else if(
                (pecas_recebidas.pecas[pecas_recebidas.next_receive - 1].altura\
                     == nova_peca.altura) \
                && \
                (pecas_recebidas.pecas[pecas_recebidas.next_receive - 2].altura\
                     = nova_peca.altura))
            {
                nova_peca.valid = false;
                pecas_recebidas.count_pecas++;
                /*  salva informacoes da peca atual no vetor de pecas recebidas 
                */
                pecas_recebidas.pecas[pecas_recebidas.next_receive].altura = \
                    nova_peca.altura;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].timer = \
                    nova_peca.timer;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].valid = \
                    nova_peca.valid;
                pecas_recebidas.pecas[pecas_recebidas.next_receive].posicao = \
                    nova_peca.posicao;
            }
        break;
    }
    
}

void Ativar_Solenoide(uint8_t solenoide)
{
    switch (solenoide)
    {
    case 1:
        digitalWrite(SOLENOIDE_1_A, HIGH);

        delay(200);

        digitalWrite(SOLENOIDE_1_A, LOW);
        digitalWrite(SOLENOIDE_1_R, HIGH);

        delay(200);

        digitalWrite(SOLENOIDE_1_R, LOW);
        break;

    case 2:
        digitalWrite(SOLENOIDE_2, HIGH);

        delay(200);

        digitalWrite(SOLENOIDE_2, LOW);
        break;

    case 3:
        digitalWrite(SOLENOIDE_3, HIGH);

        delay(200);

        digitalWrite(SOLENOIDE_3, LOW);
        break;
    }
}
