/***********************************************************************************
 *  An bot that receives commands from telegram and                                *
 *  detects the use of the helmet and its temperature                              *
 *  written by Adriel Humberto                                                     *
 *  refers: https://www.filipeflop.com/blog/programacao-esp8266-ota-wifi/          *
 *  refers: http://www.fernandok.com/2017/12/sensor-infravermelho-com-esp8266.html *
 *  refers: http://blog.eletrogate.com/nodemcu-esp12-alarme-residencial-iot-3/     *
 **********************************************************************************/

#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h> // I2C library, required for MLX90614
#include <SparkFunMLX90614.h> // SparkFunMLX90614 Arduino library

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>


// pinagem para o NodeMCU ESP8266
#define sclk D5
#define mosi D7
#define cs   D8
#define rst  D3
#define dc   D4

// Inicializa o BOT Telegram - copie aqui a chave Token quando configurou o seu BOT - entre aspas
#define BOTtoken "561703826:AAHMAXFRqLGyp0irdrXVsdHUXJXt1Ty1weg"  // sua chave Token Bot

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 1000;      // tempo entre a leitura das mensagens
long Bot_lasttime;        // ultima mensagem lida
bool Start = false;
bool capaceteBusy = false;
int contBusy = 0;
String lastChatId = "607902099";
//objeto responsável pela comunicação com o sensor infravermelho
IRTherm sensor;

//variáveis que armazenarão o valor das temperaturas lidas
float tempAmbiente;
float tempObjeto;


const char* ssid = "capacete";
const char* password = "capacete123";
 
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Conexao falhou! Reiniciando...");
    delay(5000);
    ESP.restart();
  }
 
  // Porta padrao do ESP8266 para OTA eh 8266 - Voce pode mudar ser quiser, mas deixe indicado!
  // ArduinoOTA.setPort(8266);
 
  // O Hostname padrao eh esp8266-[ChipID], mas voce pode mudar com essa funcao
  // ArduinoOTA.setHostname("nome_do_meu_esp8266");
 
  // Nenhuma senha eh pedida, mas voce pode dar mais seguranca pedindo uma senha pra gravar
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });

  Serial.println("Pronto");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.begin();

  //Inicializa sensor de temperatura infravermelho
  sensor.begin();

  //Seleciona temperatura em Celsius
  sensor.setUnit(TEMP_C);//podemos ainda utilizar TEMP_F para Fahrenheit  
}
 
void loop() {
  ArduinoOTA.handle();

  if (millis() > Bot_lasttime + Bot_mtbs)    // controlando as mensagens
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
 
    while (numNewMessages)                  // numero de novas mensagens
    {
      Serial.println("Resposta recebida do Telegram");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
 
    Bot_lasttime = millis();
  }

  if (sensor.read())
  {
    //recupera a leitura da temperatura do ambiente
    tempAmbiente = sensor.ambient();
     
    //recupera a leitura da temperatura do objeto apontado pelo sensor
    tempObjeto = sensor.object(); 

    float x = tempObjeto - tempAmbiente;
    if((x > 1 || x < -1) && tempObjeto > 30) {
      if(capaceteBusy == false) {
        bot.sendMessage(lastChatId, "O capacete voltou a ser utilizado :)", "");  // envia mensagem  
        capaceteBusy = true;
        contBusy=0;
      }
      
    }
    else {
      if(contBusy < 15) {
        contBusy+=1;
        delay(1000);
      }
      else if(contBusy = 15) {
        contBusy=0;
        capaceteBusy = false;
        bot.sendMessage(lastChatId, "O capacete não está sendo utilizado, por favor verifique.", "");  // envia mensagem  
        delay(1000);
      }
    }
  }
  
  delay(50);
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("Mensagem recebida = ");
  Serial.println(String(numNewMessages));
 
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
 
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";
 
    if (text == "/checar")                             
    {

      if (capaceteBusy)
      { 
        bot.sendMessage(chat_id, "O capacete está em uso :)", "");  // envia mensagem
      }
      else
      {
        bot.sendMessage(chat_id, "O capacete não está sendo utilizado, por favor verifique.", "");  // envia mensagem  
      }
      
    }

    if (text == "/temperatura")                             
    {

        String msg;
        msg += F("A temperatura do trabalhador aproximada é: ");
        msg += String(tempObjeto, 2);
        msg += F("°C \n, Já a temperatura ambiente atual é: ");
        msg += String(tempAmbiente, 2);
        msg += F("°C.");
        
        bot.sendMessage(chat_id, msg, "");  // envia mensagem
           
    }

    if (text == "/start")                              
    {
      String welcome = "Olá eu sou o fiscal da obra, sou responsável por fiscalizar tudo por aqui :), " + from_name + ".\n";
      welcome += "/checar : para verificar o uso do capacete \n";
      welcome += "/temperatura : mostra as condições do capacete \n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}
