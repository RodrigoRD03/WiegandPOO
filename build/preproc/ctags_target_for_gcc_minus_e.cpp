# 1 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino"
# 2 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 3 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 4 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 5 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 6 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 7 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2
# 8 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino" 2

int lcdColumns = 16, lcdRows = 2;
String RFID, contrasena;
const char* ssid = "TP-LINK_74DE";
const char* pass = "55242103";

Preferences preferences;
WIEGAND wg;
HTTPClient http;
WiFiClient espClient;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);
FirebaseData firebaseData;

// Pines de salida para las cajas
# 33 "c:\\Users\\Equipo 25\\Documents\\Arduino\\WiegandOOP\\WiegandPoo.ino"
// Variables de entrada para Firebase REALTIME DATABASE



class Caja {
    private:
        uint8_t Puerta;
        uint8_t caja1;
        uint8_t caja2;
        uint8_t caja3;
        uint8_t caja4;

    public:
        void definirSalidas(uint8_t puerta, uint8_t box1, uint8_t box2, uint8_t box3, uint8_t box4){
            Puerta = puerta;
            caja1 = box1;
            caja2 = box2;
            caja3 = box3;
            caja4 = box4;
        }

        void abrirPuerta(){
            digitalWrite(Puerta, 0x0);
        }

        void abrirCajas(int caja){
            uint8_t cajaAbrir;
            if(caja == 1){
                cajaAbrir = caja1;
            } else if (caja == 2){
                cajaAbrir = caja2;
            } else if(caja == 3){
                cajaAbrir = caja3;
            } else if(caja == 4){
                cajaAbrir = caja4;
            } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                Serial.println("Numero de caja invalida");
            }
            digitalWrite(cajaAbrir, 0x0);
            delay(5000);
            digitalWrite(cajaAbrir, 0x1);
        }
};

Caja C1, C2;

bool flag = false;

int opcion;

void setup() {
    Serial.begin(115200); //Inicializa puerto serial
    wg.begin(15, 4);
    lcd.init();
    lcd.backlight();
    Firebase.begin("https://beniplas-643b4-default-rtdb.firebaseio.com/", "ljyYjowW3ng0NI9ceQNx15Kl2DCWR0gHqvgBzSyg");
    lcd.clear();
    lcd.setCursor(0, 0);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){ // Conectar el ESP32 a Internet
        delay(500);
        lcd.print(".");
    }
    MostrarTexto("WI-FI Conectado", " ");
    delay(1000);
    uint8_t entradas[] = { 16, 13 , 17, 5, 18, 19, 12, 14, 27, 26 };
    for(int i = 0; i < 10; i++){
        pinMode(entradas[i], 0x03);
        digitalWrite(entradas[i], 0x1);
    }
    preferences.begin("Beniplas", false);
    if(preferences.getInt("contador", 0) == 0){
        preferences.putInt("contador", false);
    }
    preferences.end();
    C1.definirSalidas(16, 17, 5, 18, 19);
    C2.definirSalidas(13, 12, 14, 27, 26);
}

void loop() {
    int lado;
    while(!flag){
        MostrarTexto("    BENIPLAS    ", " Pulse un Boton ");
        delay(500);
        if(wg.available()) {
            flag = true;
        }
    }
    MostrarTexto("Selec. Opcion", "Puerta Izq .. 1");
    delay(2000);
    MostrarTexto("Puerta Der .. 2", "Registrar ... 3");
    while(flag){
        if(wg.available()){
            flag = false;
        }
    }
    switch (wg.getCode())
    {
    case 1:
        ProcesoPuertas();
        delay(3000);
        break;
    case 2:
        ProcesoPuertas();
        delay(3000);
        break;
    case 3:
        MostrarTexto("Opcion 3", " ");
        delay(3000);
        break;
    case 4:
        MostrarTexto("Opcion 4", " ");
        delay(3000);
        break;
    case 27:
        break;
    default:
        lcd.print("Opcion Invalida");
        break;
    }
}

void ProcesoPuertas(){
    String topico;
    SolicitarRFID();
    if(flag){
        CrearContrasena();
        topico = ObtenerTopico();
        EnviarNotificaciones(topico, contrasena);
        IngresarContasenaPuertas();
    }
}

void MostrarTexto(String superior, String inferior){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(superior);
    lcd.setCursor(0, 1);
    lcd.print(inferior);
}

void SolicitarRFID() {
    RFID = "";
    unsigned long tiempoEspera = 0;
    MostrarTexto("Ingrese su RFID", " ");
    while(RFID.length() != 7){
        if(wg.available()){
            RFID = wg.getCode();
            flag = true;
        }
        if(tiempoEspera == 0){
            tiempoEspera = millis();
        }
        if(millis() > tiempoEspera + 30000){
            MostrarTexto("Tiempo de espera", "alcanzado");
            flag = false;
            break;
        }
    }
}


void CrearContrasena(){ // Crea una seria de numeros aleatorios para conforma una contraseña de 6 digitos la cual se pedira a la hora de abrir las puertas o las cajas
    contrasena = "";
    MostrarTexto("Creando PASS", "");
    for(int i = 0; i != 6; i++){
        int Random = random(0, 9);
        contrasena += (String)Random;
    }
    Serial.println("La contrasana es: " + contrasena);
}

String ObtenerTopico(){
    String topico;
    preferences.begin("Beniplas", false);
    String registrosRFID[] = {preferences.getString("Espacio1_RFID", ""), preferences.getString("Espacio2_RFID", ""), preferences.getString("Espacio3_RFID", ""), preferences.getString("Espacio4_RFID", ""), preferences.getString("Espacio5_RFID", "")};
    String registrosTopicos[] = {preferences.getString("Espacio1_Topico", ""), preferences.getString("Espacio2_Topico", ""), preferences.getString("Espacio3_Topico", ""), preferences.getString("Espacio4_Topico", ""), preferences.getString("Espacio5_Topico", "")};
    for(int i = 0; i < 5; i++){
        if(RFID == registrosRFID[i]){
            topico = registrosTopicos[i];
        }
    }
    return topico;
}

void EnviarNotificaciones(String topico, String mensaje){
    lcd.clear();
    lcd.setCursor(0, 0);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){ // Conectar el ESP32 a Internet
        delay(500);
        lcd.print(".");
    }
    http.begin("https://fcm.googleapis.com/fcm/send");
    String data = "{";
    data = data + "\"to\":\"/topics/"+ topico + "\",";
    data = data + "\"notification\": {";
    data = data + "\"body\": \""+ mensaje +"\",";
    data = data + "\"title\":\"Guarda valor\"";
    data = data + "} }";
    http.addHeader("Authorization", "key=AAAA33Jn67g:APA91bEtgrlrwe0aW5YZynmTvjhj0DhpMB9H-UPXNa-2tNDOb91kpc83wvGm4minIGPPaBS1ri1zNiyIQvpbH2Dt2888fd31e9x8w45W-q86kXLWINW_vpkg9-_SrEMZ9AOlJTdFc59y");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", (String)data.length());
    int httpResponseCode = http.POST(data);
    flag = false;
}

String EliminarCaracteres(String texto){
    for(int i = 0; i < texto.length(); i++){
        if(i == texto.length() - 1){
            texto.remove(i, 1);
            MostrarTexto("Ingrese Contrasena", texto);
        }
    }
    return texto;
}

void IngresarContasenaPuertas(){
    String contrasenaIngresada = "";
    int intentos = 0;
    unsigned long tiempoEspera = millis();
    while (intentos != 3 && contrasena != contrasenaIngresada){
        contrasenaIngresada = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Ingrese Contrasena");
        while(wg.getCode() != 13){
            if(wg.available()){
                if(wg.getCode() != 13 && wg.getCode() != 27){
                    contrasenaIngresada += wg.getCode();
                    lcd.setCursor(0, 1);
                    lcd.print(contrasenaIngresada);
                }
                if(wg.getCode() == 27){
                    contrasenaIngresada = EliminarCaracteres(contrasenaIngresada);
                }
            }
            if(millis() >= (tiempoEspera + 60000)){
                MostrarTexto("Tiempo de espera", "alcanzado");
                break;
            }
        }
        if(contrasenaIngresada == contrasena){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Abriendo");
        } else {
            MostrarTexto("Contrasena", "Incorecta");
            delay(2000);
            intentos++;
            if(intentos < 3){
                MostrarTexto("Vuelve a", "Intentarlo");
            }
            MostrarTexto("Presione", "cualquier boton");
            while(wg.getCode() == 13){
                if(wg.available()){
                    wg.getCode();
                }
            }
        }

    }
}
