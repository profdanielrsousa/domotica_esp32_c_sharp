/* PROJETO FINAL CURSO CPS (PROGRAMAÇÃO C# COM ESP32)
 * SENSOR IOT DE PRESENÇA E GÁS INTELIGENTE
 * 
 * ESP32 NODEMCU - DEVKIT V1 (30 PINOS)
 * SENSOR DE GÁS (GLP E GÁS NATURAL) MQ-05
 * SENSOR DE UMIDADE RELATIVA E TEMPERATURA DHT11
 * SENSOR DE PRESENÇA PIR HC-SR501
 * BUZZER ATIVO
 * MÓDULO RELÉ (LED)
 * 
 * AUTOR: DANIEL RODRIGUES DE SOUSA 04/09/2024 */

#include <esp_task_wdt.h>     // Inclui a biblioteca Wachdog Timer
#include <DHT.h>              // Inclui a biblioteca DHT
#include <WiFi.h>             // Inclui a biblioteca WiFi para conectar o ESP32 à rede WiFi
#include <WebSocketsServer.h> // Inclui a biblioteca WebSocketsServer para criar um servidor WebSocket

// Habilita o debug pela porta serial
#define SERIAL_DEBUG

// Definição dos pinos de entrada
#define PIR_PIN     13    // Pino do sensor PIR (sensor de movimento)
#define PIN_DHT11   27    // Pino do sensor de temperatura e umidade DHT11
#define PIN_MQ05    39    // Pino do sensor de gás MQ05

// Definição dos pinos de saída
#define PIN_RELE    19    // Pino de controle do relé
#define PIN_BUZZER  23    // Pino de controle do buzzer

// Definição das credenciais da rede WiFi
#define SSID_NAME     "YOUR_SSID"      // Nome da rede WiFi
#define SSID_PASSWORD "YOUR_PASSWORD"  // Senha da rede WiFi

#define VALOR_GAS   3000  // Limite de detecção de gás para acionamento do alarme

// Declaração de variáveis
unsigned long int tempo_atual;  // Variável para armazenar o tempo atual
unsigned long int tempo_100ms;  // Temporizador para intervalos de 100ms
unsigned long int tempo_500ms;  // Temporizador para intervalos de 500ms

// Enumeração dos sensores
enum
{
  SENSOR_GAS = 0,         // Sensor de gás
  SENSOR_TEMPERATURA,     // Sensor de temperatura
  SENSOR_UMIDADE,         // Sensor de umidade
  SENSOR_PIR,             // Sensor PIR (movimento)
} sm_sensor = SENSOR_GAS; // Inicia o estado da máquina no sensor de gás

String leituraString = "";// String para armazenar leituras dos sensores

float temperatura;        // Armazena a leitura da temperatura
float umidade;            // Armazena a leitura da umidade
int nivel_gas;            // Armazena o nível de gás detectado
bool f_liga_rele;         // Flag para controle manual do relé
bool f_modo_automatico;   // Flag para ativação do modo automático
bool f_pir;               // Flag para detecção de movimento pelo sensor PIR

#define DHTTYPE     DHT11 // Define o tipo do sensor DHT como DHT11

DHT dht(PIN_DHT11, DHTTYPE);  // Instancia o sensor DHT11
WebSocketsServer webSocket = WebSocketsServer(81);  // Cria uma instância do servidor WebSocket na porta 81

// Configuração do watchdog (timer de segurança)
esp_task_wdt_config_t wdt_config = {
  .timeout_ms = 4000,   // Define o tempo limite de 4 segundos para o watchdog
  .idle_core_mask = 0,  // Define a máscara de núcleo ocioso
  .trigger_panic = true // Define para acionar pânico em caso de timeout
};

void setup()
{
  // Configura os pinos como saída
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Configura os pinos como entrada
  pinMode(PIR_PIN, INPUT);
  pinMode(PIN_DHT11, INPUT);
  pinMode(PIN_MQ05, INPUT);

  #if defined SERIAL_DEBUG
    Serial.begin(115200); // Inicializa a comunicação serial a 115200 bps para debug
    Serial.flush();       // Limpa a serial
  #endif

  dht.begin(); // Inicializa o sensor DHT11

  // Habilita o watchdog configurando o timeout para 4 segundos
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  WiFi.begin(SSID_NAME, SSID_PASSWORD); // Conecta-se à rede WiFi usando o SSID e a senha fornecidos

  while(WiFi.status() != WL_CONNECTED)
  { // Loop até que a conexão WiFi seja estabelecida
    delay(1000); // Espera 1 segundo
    esp_task_wdt_reset();     // Reseta o watchdog timer

    #if defined SERIAL_DEBUG
      Serial.println("Conectando no WiFi..."); // Imprime mensagem de conexão ao WiFi
    #endif
  }
  #if defined SERIAL_DEBUG
    Serial.println("Conectou no WiFi"); // Imprime mensagem de conexão bem-sucedida ao WiFi

    // Exibe o endereço IP do ESP32
    Serial.print("IP do ESP32 na Rede Conectada: ");  // Imprime rótulo para o endereço IP
    Serial.println(WiFi.localIP()); // Imprime o endereço IP local do ESP32
  #endif

  webSocket.begin(); // Inicia o servidor WebSocket
  webSocket.onEvent(webSocketEvent); // Define a função de callback para eventos do WebSocket

  f_liga_rele = 0;        // Inicializa o relé como desligado
  f_modo_automatico = 0;  // Inicializa o modo automático como desligado
  f_pir = 0;              // Inicializa a flag de detecção PIR como falsa
}

void loop()
{
  tempo_atual = millis(); // Atualiza o tempo atual

  #if defined SERIAL_DEBUG
    if((tempo_atual - tempo_100ms) >= 250)  // Temporização a cada 250ms (modo de depuração)
  #else
    if((tempo_atual - tempo_100ms) >= 100)  // Temporização a cada 100ms
  #endif
  {
    tempo_100ms = millis(); // Recarrega o temporizador de 100ms

    switch(sm_sensor)
    {
      case SENSOR_GAS: // Leitura do sensor de gás
        nivel_gas = analogRead(PIN_MQ05);
        sm_sensor = SENSOR_TEMPERATURA; // Próximo sensor: temperatura
        #if defined SERIAL_DEBUG
          Serial.printf("Nivel gas: %d\n", nivel_gas);
        #endif
        break;

      case SENSOR_TEMPERATURA: // Leitura do sensor de temperatura
        temperatura = dht.readTemperature();
        sm_sensor = SENSOR_UMIDADE; // Próximo sensor: umidade
        #if defined SERIAL_DEBUG
          Serial.printf("Temperatura: %2.1f\n", temperatura);
        #endif
        break;

      case SENSOR_UMIDADE: // Leitura do sensor de umidade
        umidade = dht.readHumidity();
        sm_sensor = SENSOR_PIR; // Próximo sensor: PIR (movimento)
        #if defined SERIAL_DEBUG
          Serial.printf("Umidade: %2.1f\n", umidade);
        #endif
        break;

      case SENSOR_PIR: // Leitura do sensor PIR (movimento)
        f_pir = digitalRead(PIR_PIN);
        sm_sensor = SENSOR_GAS; // Retorna para o sensor de gás
        #if defined SERIAL_DEBUG
          Serial.printf("PIR: %d\n", f_pir);
        #endif
        break;   
    }
  }

  if((tempo_atual - tempo_500ms) >= 500) // Temporização a cada 500ms
  {
    tempo_500ms = millis();   // Recarrega o temporizador de 500ms
    esp_task_wdt_reset();     // Reseta o watchdog timer

    if(nivel_gas > VALOR_GAS) // Verifica se o nível de gás excede o limite
    {
      digitalWrite(PIN_BUZZER, !digitalRead(PIN_BUZZER)); // Altera o estado do buzzer (liga/desliga)
      webSocket.broadcastTXT("Vazando gas!"); // Envia mensagem de alerta via WebSocket
    }
    else
    {
      digitalWrite(PIN_BUZZER, LOW);    // Desliga o buzzer se o nível de gás estiver normal
      webSocket.broadcastTXT("Gas ok!");// Limpa a mensagem de alerta via WebSocket
    }

    if(f_modo_automatico) // Verifica se o modo automático está ativado
    {
      digitalWrite(PIN_RELE, f_pir);      // Controla o relé baseado na detecção PIR
    }
    else
    {
      digitalWrite(PIN_RELE, f_liga_rele);// Controla o relé manualmente
    }
  }

  webSocket.loop(); // Mantém o servidor WebSocket em execução, verificando por novas conexões e mensagens
}

// Função de callback para eventos do WebSocket
void webSocketEvent(uint8_t numero_msg, WStype_t type, uint8_t * conteudo_recebido, size_t tamanho_conteudo)
{
  switch(type)
  { // Verifica o tipo de evento
    case WStype_DISCONNECTED: // Caso a conexão seja desconectada
      #if defined SERIAL_DEBUG
        Serial.printf("[%u] Desconectou!\n", numero_msg); // Imprime no serial que o cliente numero_msg foi desconectado
      #endif
      break;

    case WStype_CONNECTED: // Caso uma nova conexão seja estabelecida
      #if defined SERIAL_DEBUG
        Serial.printf("[%u] Conectou!\n", numero_msg); // Imprime no serial que o cliente numero_msg foi conectado
      #endif
      webSocket.sendTXT(numero_msg, "Conectou"); // Envia mensagem de confirmação de conexão ao cliente
      break;

    case WStype_TEXT: // Caso uma mensagem de texto seja recebida
      #if defined SERIAL_DEBUG
        Serial.printf("[%u] Recebeu pelo Socket o texto: %s\n", numero_msg, conteudo_recebido); // Imprime no serial a mensagem recebida do cliente numero_msg
      #endif

      if(strcmp((char *)conteudo_recebido, "AUTOMATICO_on") == 0)
      { // Se a mensagem for "AUTOMATICO_on"
        f_modo_automatico = 1; // Liga o modo automático
        webSocket.sendTXT(numero_msg, "Modo automático ligado"); // Envia mensagem de confirmação ao cliente
        #if defined SERIAL_DEBUG
          Serial.println("Modo automático ligado");
        #endif
      }
      else if(strcmp((char *)conteudo_recebido, "AUTOMATICO_off") == 0) 
      { // Se a mensagem for "AUTOMATICO_off"
        f_modo_automatico = 0; // Desliga o modo automático
        webSocket.sendTXT(numero_msg, "Modo automático desligado"); // Envia mensagem de confirmação ao cliente
        #if defined SERIAL_DEBUG
          Serial.println("Modo automático desligado");
        #endif
      }
      else if(strcmp((char *)conteudo_recebido, "RELE_on") == 0) 
      { // Se a mensagem for "RELE_on"
        f_liga_rele = 1; // Liga o relé manualmente
        webSocket.sendTXT(numero_msg, "RELE Ligado"); // Envia mensagem de confirmação ao cliente
        #if defined SERIAL_DEBUG
          Serial.println("RELE Ligado");
        #endif
      }
      else if(strcmp((char *)conteudo_recebido, "RELE_off") == 0) 
      { // Se a mensagem for "RELE_off"
        f_liga_rele = 0; // Desliga o relé manualmente
        webSocket.sendTXT(numero_msg, "RELE Desligado"); // Envia mensagem de confirmação ao cliente
        #if defined SERIAL_DEBUG
          Serial.println("RELE Desligado");
        #endif
      }
      else if(strcmp((char *)conteudo_recebido, "TEMPERATURA") == 0) 
      { // Se a mensagem for "TEMPERATURA"
        webSocket.sendTXT(numero_msg, "Sensor de temperatura"); // Envia mensagem de confirmação ao cliente
        leituraString = String((int)temperatura); // Converte a temperatura para string
        webSocket.broadcastTXT(leituraString);    // Envia a temperatura ao cliente
      }
      else if(strcmp((char *)conteudo_recebido, "UMIDADE") == 0) 
      { // Se a mensagem for "UMIDADE"
        webSocket.sendTXT(numero_msg, "Sensor de umidade"); // Envia mensagem de confirmação ao cliente
        leituraString = String((int)umidade); // Converte a umidade para string
        webSocket.broadcastTXT(leituraString);// Envia a umidade ao cliente
      }
      else if(strcmp((char *)conteudo_recebido, "PIR") == 0) 
      { // Se a mensagem for "PIR"
        webSocket.sendTXT(numero_msg, "Sensor PIR"); // Envia mensagem de confirmação ao cliente
        leituraString = String((int)f_pir);   // Converte o estado do sensor PIR para string
        webSocket.broadcastTXT(leituraString);// Envia o estado do sesor PIR ao cliente
      }
      else if(strcmp((char *)conteudo_recebido, "RELE") == 0) 
      { // Se a mensagem for "RELE"
        webSocket.sendTXT(numero_msg, "Estado saida rele"); // Envia mensagem de confirmação ao cliente
        leituraString = String((int)digitalRead(PIN_RELE)); // Converte o estado do rele para string
        webSocket.broadcastTXT(leituraString); // Envia a o estado do rele ao cliente
      }         
      break;
  }
}
