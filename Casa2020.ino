#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>

#define pinLuzPia 13
#define pinLuzSala 12
#define pinChuveiro 14
#define pinLuzBanheiro 27
#define pinLuzQ1 26
#define pinLuzQ2 25
#define pinLuzCozinha 33
#define pinLuzMesa 32
#define sensorPE 35  // Sensor Presenca Escada
#define sensorIRC 34  // Sensor Infra Red Chegada
#define sensorPS 39  // Sensor Infra Red saida
#define sensorPB 36 // Sensor Presenca Banheiro
#define pinLuzEscada 5
#define pinPortaEntrada 17

unsigned long tempoSB = 0;
boolean sensorBanheiroOn = true;
boolean sensorEscadaOn = true;
boolean LuzBanheiroOnApp = false;
boolean luzbanhChuv = false;
boolean saidaBan = false;
boolean luzEscApp = false;
boolean luzCozApp = false;
boolean sensChegAtual = false;
boolean sensChegAnterior = false;
boolean iRC = false;

const char* ssid = "Thuliv";
const char* password = "90iojknm";

//Parâmetros de rede
IPAddress ip(192, 168, 1, 254);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

//Algumas informações que podem ser interessantes
const uint32_t chipID = (uint32_t)(ESP.getEfuseMac() >> 32); //um ID exclusivo do Chip...
const String CHIP_ID = "<p> Chip ID: " + String(chipID) + "</p>"; // montado para ser usado no HTML
const String VERSION = "<p> Versão: 1.0 </p>"; //Exemplo de um controle de versão

//Informações interessantes agrupadas
const String INFOS = VERSION + CHIP_ID;

//Sinalizador de autorização do OTA
boolean OTA_AUTORIZADO = false;

//inicia o servidor na porta selecionada
//aqui testamos na porta 3000, ao invés da 80 padrão
WebServer server(80);

String index1 = 
"<!DOCTYPE html>"
  "<html>"
    "<head>"
      "<title>Minha Casa</title>"
      "<meta charset='UTF-8'>"
    "</head>"
    
    "<body>"
      "<h1>Minha Casa</h1>"
      + INFOS +
      "<form method='POST' action='/arquivo' enctype='multipart/form-data'>"
      "<label>Chave: </label><input type='text' name='autorizacao'> <input type='submit'value='Ok'></form>"
      
      "<p><form method='POST' action='/lLPia'><button>Ligar Luz da Pia</button></form></p>"
      "<p><form method='POST' action='/dLPia'><button>Desligar Luz da Pia</button></form></p>"
      
      "<p><form method='POST' action='/lChuveiro'><button>Ligar Chuveiro</button></form></p>" 
      "<p><form method='POST' action='/dChuveiro'><button>Desligar Chuveiro</button></form></p>"

      "<p><form method='POST' action='/lLSala'><button>Ligar Luz da Sala</button></form></p>" 
      "<p><form method='POST' action='/dLSala'><button>Desligar Luz da Sala</button></form></p>"

      "<p><form method='POST' action='/lLBanheiro'><button>Ligar Luz do Banheiro</button></form></p>" 
      "<p><form method='POST' action='/dLBanheiro'><button>Desligar Luz do Banheiro</button></form></p>"

      "<p><form method='POST' action='/lLQ1'><button>Ligar Luz Quarto 1</button></form></p>" 
      "<p><form method='POST' action='/dLQ1'><button>Desligar Luz Quarto 1</button></form></p>"
      
      "<p><form method='POST' action='/lLQ2'><button>Ligar Luz Quarto 2</button></form></p>" 
      "<p><form method='POST' action='/dLQ2'><button>Desligar Luz Quarto 2</button></form></p>" 

      "<p><form method='POST' action='/lLCozinha'><button>Ligar Luz da Cozinha</button></form></p>" 
      "<p><form method='POST' action='/dLCozinha'><button>Desligar Luz da Cozinha</button></form></p>"

      "<p><form method='POST' action='/lLMesa'><button>Ligar Luz da Mesa</button></form></p>" 
      "<p><form method='POST' action='/dLMesa'><button>Desligar Luz da Mesa</button></form></p>"

      "<p><form method='POST' action='/lLEscada'><button>Ligar Luz da Escada</button></form></p>" 
      "<p><form method='POST' action='/dLEscada'><button>Desligar Luz da Escada</button></form></p>"
    "</body>"  
  "</html>";
String index2 = "<!DOCTYPE html><html><head><title>Minha Casa</title><meta charset='UTF-8'></head><body><h1>Minha Casa</h1>"+ INFOS +"<form method='POST'action='/update' enctype='multipart/form-data'><p><input type='file' name='update'></p><p><input type='submit' value='Atualizar'></p></form</body></html>";
String atualizado = "<!DOCTYPE html><html><head><title>Minha Casa</title><meta charset='UTF-8'></head><body><h1>Minha Casa</h1><h2>Atualização bem sucedida!</h2></body></html>";
String chaveIncorreta = "<!DOCTYPE html><html><head><title>Minha Casa</title><meta charset='UTF-8'></head><body><h1>Minha Casa</h1>"+ INFOS +"<h2>Chave incorreta</h2</body></html>";

hw_timer_t *timer = NULL; 

void IRAM_ATTR resetModule(){
   esp_restart(); //reinicia o chip
}


void setup(void)
{
  pinMode(pinLuzPia, OUTPUT);
  pinMode(pinLuzSala, OUTPUT);
  pinMode(pinChuveiro, OUTPUT);
  pinMode(pinLuzBanheiro, OUTPUT);
  pinMode(pinLuzQ1, OUTPUT);
  pinMode(pinLuzQ2, OUTPUT);
  pinMode(pinLuzCozinha, OUTPUT);
  pinMode(pinLuzMesa, OUTPUT); 
  pinMode(pinLuzEscada, OUTPUT);
  pinMode(sensorPB, INPUT);
  pinMode(sensorPE, INPUT);
  pinMode(sensorIRC, INPUT);
  pinMode(sensorPS, INPUT);

  digitalWrite(pinLuzPia, HIGH);
  digitalWrite(pinLuzSala, HIGH);
  digitalWrite(pinChuveiro, LOW);
  digitalWrite(pinLuzBanheiro, HIGH);
  digitalWrite(pinLuzQ1, HIGH);
  digitalWrite(pinLuzQ2, HIGH);
  digitalWrite(pinLuzCozinha, HIGH);
  digitalWrite(pinLuzMesa, HIGH);
  digitalWrite(pinLuzEscada, HIGH);
  
  Serial.begin(115200); //Serial para debug

  WiFi.mode(WIFI_AP_STA); //Comfigura o ESP32 como ponto de acesso e estação

  WiFi.begin(ssid, password);// inicia a conexão com o WiFi

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.config(ip, gateway, subnet);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (WiFi.status() == WL_CONNECTED) //aguarda a conexão
  {
    //atende uma solicitação para a raiz
    // e devolve a página 'verifica'
    server.on("/", HTTP_GET, []() //atende uma solicitação para a raiz
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
    });

    //atende uma solicitação para a página avalia
    server.on("/arquivo", HTTP_POST, [] ()
    {
      Serial.println("Em server.on /avalia: args= " + String(server.arg("autorizacao"))); //somente para debug

      if (server.arg("autorizacao") != "90iojknm") // confere se o dado de autorização atende a avaliação
      {
        //se não atende, serve a página indicando uma falha
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", chaveIncorreta);
        //ESP.restart();
      }
      else
      {
        //se atende, solicita a página de índice do servidor
        // e sinaliza que o OTA está autorizado
        OTA_AUTORIZADO = true;
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", index2);
      }
    });

    //serve a página de indice do servidor
    //para seleção do arquivo
    server.on("/index2", HTTP_GET, []()
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index2);
    });

    //tenta iniciar a atualização . . .
    server.on("/update", HTTP_POST, []()
    {
      //verifica se a autorização é false.
      //Se for falsa, serve a página de erro e cancela o processo.
      if (OTA_AUTORIZADO == false)
      {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", chaveIncorreta);
        return;
      }
      //Serve uma página final que depende do resultado da atualização
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", (Update.hasError()) ? chaveIncorreta : atualizado);
      delay(1000);
      ESP.restart();
    }, []()
    {
      //Mas estiver autorizado, inicia a atualização
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START)
      {
        Serial.setDebugOutput(true);
        Serial.printf("Atualizando: %s\n", upload.filename.c_str());
        if (!Update.begin())
        {
          //se a atualização não iniciar, envia para serial mensagem de erro.
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE)
      {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
          //se não conseguiu escrever o arquivo, envia erro para serial
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END)
      {
        if (Update.end(true))
        {
          //se finalizou a atualização, envia mensagem para a serial informando
          Serial.printf("Atualização bem sucedida! %u\nReiniciando...\n", upload.totalSize);
        }
        else
        {
          //se não finalizou a atualização, envia o erro para a serial.
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      else
      {
        //se não conseguiu identificar a falha no processo, envia uma mensagem para a serial
        Serial.printf("Atualização falhou inesperadamente! (possivelmente a conexão foi perdida.): status=%d\n", upload.status);
      }
    });

    ////////////ligar luz da pia///////////////////////

    server.on("/lLPia", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzPia, LOW);
    });
    
    server.on("/lLPia", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzPia, LOW);
    });

///////////desligar luz da pia/////////////////////

    server.on("/dLPia", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzPia, HIGH);
    });
    
    server.on("/dLPia", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzPia, HIGH);
    });

///////////ligar chuveiro/////////////

    server.on("/lChuveiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinChuveiro, HIGH);
    });
    
    server.on("/lChuveiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinChuveiro, HIGH);
    });


//////////desligar chuveiro///////////

    server.on("/dChuveiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinChuveiro, LOW);
    });
    
    server.on("/dChuveiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinChuveiro, LOW);
    });

//////////ligar luz da sala///////////

    server.on("/lLSala", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzSala, LOW);
    });
    
    server.on("/lLSala", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzSala, LOW);
    });

//////////desligar luz da sala///////////

    server.on("/dLSala", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzSala, HIGH);
    });
    
    server.on("/dLSala", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzSala, HIGH);
    });

//////////ligar luz do banheiro///////////

    server.on("/lLBanheiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzBanheiro, LOW);
    });
    
    server.on("/lLBanheiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzBanheiro, LOW);
    });

//////////desigar luz do banheiro///////////

    server.on("/dLBanheiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzBanheiro, HIGH);
    });
    
    server.on("/dLBanheiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzBanheiro, HIGH);
    });

//////////ligar luz Q1///////////

    server.on("/lLQ1", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ1, LOW);
    });
    
    server.on("/lLQ1", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ1, LOW);
    });

//////////desligar luz Q1///////////

    server.on("/dLQ1", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ1, HIGH);
    });
    
    server.on("/dLQ1", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ1, HIGH);
    });

//////////ligar luz Q2///////////

    server.on("/lLQ2", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ2, LOW);
    });
    
    server.on("/lLQ2", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ2, LOW);
    });

//////////desligar luz Q2///////////

    server.on("/dLQ2", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ2, HIGH);
    });
    
    server.on("/dLQ2", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzQ2, HIGH);
    });

//////////ligar luz da cozinha///////////

    server.on("/lLCozinha", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzCozinha, LOW);
    });
    
    server.on("/lLCozinha", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzCozinha, LOW);
    });

//////////desligar luz da cozinha///////////

    server.on("/dLCozinha", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzCozinha, HIGH);
    });
    
    server.on("/dLCozinha", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzCozinha, HIGH);
    });

//////////ligar luz da mesa///////////

    server.on("/lLMesa", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzMesa, LOW);
    });
    
    server.on("/lLMesa", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzMesa, LOW);
    });

//////////desligar luz da mesa///////////

    server.on("/dLMesa", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzMesa, HIGH);
    });
    
    server.on("/dLMesa", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzMesa, HIGH);
    });

//////////ligar luz da escada///////////

    server.on("/lLEscada", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzEscada, LOW);
    });
    
    server.on("/lLEscada", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzEscada, LOW);
    });

//////////desligar luz da escada///////////

    server.on("/dLEscada", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzEscada, HIGH);
    });
    
    server.on("/dLEscada", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      digitalWrite (pinLuzEscada, HIGH);
    });

    server.begin(); //inicia o servidor
  }

  timer = timerBegin(0, 80, true); 
  
  timerAttachInterrupt(timer, &resetModule, true);
   
  timerAlarmWrite(timer, 30000000, true);
  timerAlarmEnable(timer); 
}


void loop(void)
{
  server.handleClient();
  timerWrite(timer, 0);
}
