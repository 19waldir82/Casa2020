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

WebServer server(80);

String index1 = 
"<!DOCTYPE html>"
  "<html>"
    "<head>"
      "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/>"
      "<title>ThulivTEC Home</title>"
      "<meta charset='UTF-8'>"

      "<style>"
        "body{"
          "text-align: center;"
          "font-family: sans-serif;"
          "font-size:14px;"
          "padding: 25px;"
        "}"

        "p{"
          "color:#444;"
        "}"

        "button{"
          "outline: none;"
          "border: 2px solid #1fa3ec;"
          "border-radius:18px;"
          "background-color:#FFF;"
          "color: #1fa3ec;"
          "padding: 10px 50px;"
        "}"

        "button:active{"
          "color: #FFF;"
          "background-color:#F60;"
        "}"
      "</style>"
    "</head>"
    
    "<body>"
      "<h1>ThulivTEC Home</h1>"
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
String index2 = "<!DOCTYPE html><html><head><title>ThulivTEC Home</title><meta charset='UTF-8'></head><body><h1>ThulivTEC Home</h1>"+ INFOS +"<form method='POST'action='/update' enctype='multipart/form-data'><p><input type='file' name='update'></p><p><input type='submit' value='Atualizar'></p></form</body></html>";
String atualizado = "<!DOCTYPE html><html><head><title>ThulivTEC Home</title><meta charset='UTF-8'></head><body><h1>ThulivTEC Home</h1><h2>Atualização bem sucedida!</h2></body></html>";
String chaveIncorreta = "<!DOCTYPE html><html><head><title>ThulivTEC Home</title><meta charset='UTF-8'></head><body><h1>ThulivTEC Home</h1>"+ INFOS +"<h2>Chave incorreta</h2</body></html>";

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
  
  Serial.begin(115200); 

  WiFi.mode(WIFI_AP_STA); 

  WiFi.begin(ssid, password);


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

  if (WiFi.status() == WL_CONNECTED) 
  {
    server.on("/", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
    });

    server.on("/arquivo", HTTP_POST, [] ()
    {
      Serial.println("Em server.on /avalia: args= " + String(server.arg("autorizacao"))); 

      if (server.arg("autorizacao") != "90iojknm") 
      {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", chaveIncorreta);
      }
      else
      {
        OTA_AUTORIZADO = true;
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", index2);
      }
    });

    server.on("/index2", HTTP_GET, []()
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index2);
    });

    server.on("/update", HTTP_POST, []()
    {

      if (OTA_AUTORIZADO == false)
      {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", chaveIncorreta);
        return;
      }

      server.sendHeader("Connection", "close");
      server.send(200, "text/html", (Update.hasError()) ? chaveIncorreta : atualizado);
      delay(1000);
      ESP.restart();
    }, []()
    {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START)
      {
        Serial.setDebugOutput(true);
        Serial.printf("Atualizando: %s\n", upload.filename.c_str());
        if (!Update.begin())
        {
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE)
      {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END)
      {
        if (Update.end(true))
        {
          Serial.printf("Atualização bem sucedida! %u\nReiniciando...\n", upload.totalSize);
        }
        else
        {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      else
      {
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
      chuveiro(true);
     luzbanhChuv = true;
     saidaBanho(false);
     LuzBanheiroOnApp = false;
     sensorBanheiroOn = false;
    });
    
    server.on("/lChuveiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      chuveiro(true);
     luzbanhChuv = true;
     saidaBanho(false);
     LuzBanheiroOnApp = false;
     sensorBanheiroOn = false;
    });


//////////desligar chuveiro///////////

    server.on("/dChuveiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      chuveiro(false);
      saidaBanho(true);
      luzbanhChuv = false;
    });
    
    server.on("/dChuveiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      chuveiro(false);
      saidaBanho(true);
      luzbanhChuv = false;
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
      LuzBanheiroOnApp = true;
    });
    
    server.on("/lLBanheiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      LuzBanheiroOnApp = true;
    });

//////////desigar luz do banheiro///////////

    server.on("/dLBanheiro", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      LuzBanheiroOnApp = false;
    });
    
    server.on("/dLBanheiro", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      LuzBanheiroOnApp = false;
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
      luzEscApp = true;
    });
    
    server.on("/lLEscada", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      luzEscApp = true;
    });

//////////desligar luz da escada///////////

    server.on("/dLEscada", HTTP_GET, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      luzEscApp = false;
    });
    
    server.on("/dLEscada", HTTP_POST, []() 
    {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", index1);
      luzEscApp = false;
    });

    server.begin(); //inicia o servidor
  }

  timer = timerBegin(0, 80, true); 
  
  timerAttachInterrupt(timer, &resetModule, true);
   
  timerAlarmWrite(timer, 30000000, true);
  timerAlarmEnable(timer); 
}


void luzEscada(boolean onOff){
   if (onOff){
      digitalWrite (pinLuzEscada, LOW);
    }else{
      digitalWrite (pinLuzEscada, HIGH);
    }
  }

void SensorEscada(){
  boolean sensEsc;
  sensEsc = digitalRead (sensorPE);

  if (sensEsc || luzEscApp){
     luzEscada(true);
    } else {
        luzEscada(false);
 }
}


void sensorBanheiro(){
  boolean sensBanh;
  sensBanh = digitalRead (sensorPB);
  
  if (sensorBanheiroOn  && sensBanh || LuzBanheiroOnApp || luzbanhChuv || saidaBan){
        luzBanheiro(true);
  }
  else {
        luzBanheiro(false);
 }
}


void saidaBanho(boolean saidaBanho1){
  if (saidaBanho1){
     if ((millis() - tempoSB) >= 120001){
        tempoSB = millis();
        saidaBan = true;
  }
 }
}


void luzBanheiro(boolean onOff){
   if (onOff){
      digitalWrite (pinLuzBanheiro, LOW); 
    } else {
      digitalWrite (pinLuzBanheiro, HIGH);
      }
  }


void chuveiro(boolean onOff){
   if (onOff){
      digitalWrite (pinChuveiro, HIGH);
    } else {
      digitalWrite (pinChuveiro, LOW);
      saidaBanho(true);
      }
  }


void temporizador(){
  if ((millis() - tempoSB) == 120000){
     sensorBanheiroOn = true;
     saidaBan = false;
  }
}


void loop(void){
  server.handleClient();
  timerWrite(timer, 0);
  SensorEscada();
  sensorBanheiro();
  temporizador();
}
