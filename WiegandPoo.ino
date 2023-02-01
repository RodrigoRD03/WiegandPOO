#include <Wiegand.h> //Librebria para usar el lector de tarjetas de proximidad
#include <Preferences.h> //Libreria para guardar variables en la memoria no volatil del ESP32
#include <SPI.h> // Libreria para conexion con Firebase
#include <WiFi.h> //Libreria para conectar a internet
#include <HTTPClient.h>  //Libreria para usar FireBase
#include <LiquidCrystal_I2C.h> //Libreria para la pantalla lcd
#include <FirebaseESP32.h>  //Libreria para consultar la Firebase 

int lcdColumns = 16, lcdRows = 2, opcion, caja;
bool flag = false;
bool puertaI, puertaD, cajaI1, cajaI2, cajaI3, cajaI4, cajaD1, cajaD2, cajaD3, cajaD4;
String RFID, contrasena, nodo = "/Beniplas/Empresa1/20", RFIDAnterior;
const char* ssid = "TP-LINK_74DE";
const char* pass = "55242103";
unsigned long tiempoBloqueo;

Preferences preferences;
WIEGAND wg;
HTTPClient http;
WiFiClient espClient;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows); 
FirebaseData firebaseData;

// Pines de salida para las cajas
#define Puerta1 16
#define Puerta2 27
#define Caja1 17
#define Caja2 5
#define Caja3 18
#define Caja4 19
#define Caja5 26
#define Caja6 25
#define Caja7 33
#define Caja8 32

// Variables de entrada para Firebase REALTIME DATABASE
#define FIREBASE_HOST "https://beniplas-643b4-default-rtdb.firebaseio.com/" 
#define FIREBASE_AUTH "ljyYjowW3ng0NI9ceQNx15Kl2DCWR0gHqvgBzSyg"

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
            caja1 =  box1;
            caja2 = box2;
            caja3 = box3;
            caja4 = box4;
        }

        void abrirPuerta(){
            digitalWrite(Puerta, HIGH);
            delay(5000);
            digitalWrite(Puerta, LOW);
        }

        void abrirCajas(int caja){
            uint8_t cajaAbrir;
            preferences.begin("Beniplas", false);
            if(caja == 1){
                cajaAbrir = caja1;
                preferences.putInt("1", 1);
            } else if (caja == 2){
                cajaAbrir = caja2;
                preferences.putInt("2", 2);
            } else if(caja == 3){
                cajaAbrir = caja3;
                preferences.putInt("3", 3);
            } else if(caja == 4){
                cajaAbrir = caja4;
                preferences.putInt("4", 4);
            } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                Serial.println("Numero de caja invalida");
                delay(2000);
            }
            preferences.end();
            digitalWrite(cajaAbrir, HIGH);
            delay(5000);
            digitalWrite(cajaAbrir, LOW);
        }
}; 

Caja C1, C2;


void setup() {
    Serial.begin(115200); //Inicializa puerto serial
    wg.begin(15, 4);
    lcd.init();                  
    lcd.backlight();
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    lcd.clear();   
    lcd.setCursor(0, 0);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){ // Conectar el ESP32 a Internet
        delay(500);
        lcd.print(".");
    }
    MostrarTexto("WI-FI Conectado", " ");
    delay(1000);
    uint8_t entradas[] = { Puerta1, Puerta2 , Caja1, Caja2, Caja3, Caja4, Caja5, Caja6, Caja7, Caja8 };
    for(int i = 0; i < 10; i++){
        pinMode(entradas[i], OUTPUT);
        digitalWrite(entradas[i], LOW);
    }
    preferences.begin("Beniplas", false);
    if(preferences.getInt("contador", 0) == 0){
        preferences.putInt("contador", false);
    }
    preferences.end();
    C1.definirSalidas(Puerta1, Caja1, Caja2, Caja3, Caja4);
    C2.definirSalidas(Puerta2, Caja5, Caja6, Caja7, Caja8);
}

void loop() {
    int lado;
    caja = 0;
    while(!flag){
        MostrarTexto("    BENIPLAS    ", " Pulse un Boton ");
        delay(500);
        if(wg.available()) {
            flag = true;
            EliminiarLista();
        }
        RevisarBloqueos();
        EscuchaFirebase();
    }
    MostrarTexto("Selec. Opcion", "Puerta Izq .. 1");
    delay(2000);
    MostrarTexto("Puerta Der .. 2", "Registrar ... 3");
    while(flag){
        if(wg.available()){
            flag = false;
            opcion = wg.getCode();  
        }
    }
    switch (opcion)
    {
        case 1:
            ProcesoPuertas();
            break;
        case 2:
            ProcesoPuertas();
            break;
        case 3:
            MenuGuardarUsuarios();
            break;
        case 4:
            VerUsuarios();
            break;
        case 27:
            break;
        default:
            lcd.print("Opcion Invalida");
            delay(2000);
            flag = true;
            break;
    }
}

void ProcesoPuertas(){
    String topico;
    bool contrasenaCorrecta;
    SolicitarRFID();
    flag = compararRFIDBloqueo();
    if(flag){
        flag = CompararRFID();
        if(flag){
            if(flag){
                CrearContrasena();
                topico = ObtenerTopico();
                ConectarInternet();
                MostrarTexto("Enviando", "Contrasena");
                EnviarNotificaciones(topico, "Pulse la notificación para ver la contraseña, tienes 30 seg.");
                delay(30000);
                EnviarNotificaciones(topico, contrasena);
                contrasenaCorrecta = IngresarContasena();
                if(contrasenaCorrecta){
                    AbrirPuerta();
                    MenuCajas();
                }
            }
        } else {
            MostrarTexto("RFID no","registrada");
            delay(2000);
        }
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
            delay(3000);
            flag = false;
            break;
        }
    }
}


void CrearContrasena(){ // Crea una seria de numeros aleatorios para conforma una contraseña de 6 digitos la cual se pedira a la hora de abrir las puertas o las cajas
    contrasena = "";
    MostrarTexto("Codigo Enviado", "");
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

void EnviarNotificaciones(String topic, String mensaje){
    http.begin("https://fcm.googleapis.com/fcm/send");
    String data = "{";
    data = data + "\"to\":\"/topics/"+ topic + "\",";
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

bool IngresarContasena(){
    bool contrasenaCorrecta, tiempo = true;
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
                delay(2000);
                contrasenaCorrecta = false;
                tiempo = false;
                break;
            }
        }
        if(contrasenaIngresada == contrasena && tiempo == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Abriendo");
            contrasenaCorrecta = true;
        } 
        else if (contrasenaIngresada != contrasena && tiempo == true) {
            MostrarTexto("Contrasena", "Incorrecta");
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
        if(tiempo == false){
            MostrarTexto("Tiempo de espera", "alcanzado");
            delay(3000);
            break;
        }
    }
    if(intentos == 3){
        contrasenaCorrecta = false;
        BloquearUsuario();
        String topico = ObtenerTopico();
        EnviarNotificaciones(topico, "Tu RFID a sido bloqueado");
        flag = true;
    }
    return contrasenaCorrecta;
}

void AbrirPuerta(){
    if(opcion == 1){
        C1.abrirPuerta();
    } else if (opcion == 2){
        C2.abrirPuerta();
    }
}


void MenuCajas(){
    bool cajaAbierta;
    RFIDAnterior = RFID;
    while(!flag){
        MostrarTexto("Selec. Opc Caja", "1, 2, 3 o 4");
        delay(500);
        if (wg.available()){
            caja = wg.getCode();
            switch (caja)
            {
            case 1:
                cajaAbierta = CompararCajasAbiertas();
                if(cajaAbierta){
                    PreocesoCajas();
                }
                break;
            case 2:
                cajaAbierta = CompararCajasAbiertas();
                if(cajaAbierta){
                    PreocesoCajas();
                }
                break;
            case 3:
                cajaAbierta = CompararCajasAbiertas();
                if(cajaAbierta){
                    PreocesoCajas();
                }
                break;
            case 4:
                cajaAbierta = CompararCajasAbiertas();
                if(cajaAbierta){
                    PreocesoCajas();
                }
                break;
            case 27:
                flag = true;
                break;
            default:
                MostrarTexto("Opcion Invalida", "");
                delay(2000);
                break;
            }
        }
    }
    flag = false;
}

void PreocesoCajas(){
    String topicoCaja;
    bool contrasenaCorrecta;
    SolicitarRFID();
    if(RFIDAnterior == RFID){
        CrearContrasena();
        topicoCaja = ObtenerTopico();
        MostrarTexto("Enviando", "Contrasena");
        EnviarNotificaciones(topicoCaja, "Pulse la notificación para ver la contraseña, tienes 30 seg.");
        delay(20000);
        EnviarNotificaciones(topicoCaja, contrasena);
        contrasenaCorrecta = IngresarContasena();
        if(contrasenaCorrecta){
            AbrirCajas();
        }
    } else {
        MostrarTexto("El RFID debe ser", "el mismo");
        delay(3000);
        flag = false;
    }
}

void AbrirCajas(){
    if(opcion == 1){
        C1.abrirCajas(caja);
    } else if (opcion == 2){
        C2.abrirCajas(caja);
    }   
}

void VerUsuarios(){
    Serial.print("Actualmente hay ");
    int contador = ObtenerContador() - 1;
    Serial.print(contador);
    Serial.println(" personas guardadas");
    Serial.println("Menu de personas"); 
    preferences.begin("Beniplas", false);
    Serial.println("Persona Numero 1, RFID: " + preferences.getString("Espacio1_RFID", String("")) + " Codigo: " + preferences.getString("Espacio1_Topico", String("")));
    Serial.println("Persona Numero 2, RFID: " + preferences.getString("Espacio2_RFID", String("")) + " Codigo: " + preferences.getString("Espacio2_Topico", String("")));
    Serial.println("Persona Numero 3, RFID: " + preferences.getString("Espacio3_RFID", String("")) + " Codigo: " + preferences.getString("Espacio3_Topico", String("")));
    Serial.println("Persona Numero 4, RFID: " + preferences.getString("Espacio4_RFID", String("")) + " Codigo: " + preferences.getString("Espacio4_Topico", String("")));
    Serial.println("Persona Numero 5, RFID: " + preferences.getString("Espacio5_RFID", String("")) + " Codigo: " + preferences.getString("Espacio5_Topico", String("")));
    preferences.end();
    preferences.begin("Beniplas", false);
    Serial.println("Bloqueo " + preferences.getString("Bloqueo_1", String("")));
    preferences.end();
}

int ObtenerContador(){ // Función para obtener el contador para estar en constante chequeo
    int contador;
    preferences.begin("Beniplas", false);
    contador = preferences.getInt("contador", 0);
    preferences.end();
    return contador;
}

void MenuGuardarUsuarios(){
    bool comparador;
    String topico;
    MostrarTexto("Nuevo Usuario 1", "Sobrescribir 2");
    flag = true;
    while(flag){
        if(wg.available()){
            switch (wg.getCode())
            {   
            case 1:
                SolicitarRFID();
                if(flag){
                    comparador = CompararRFID();
                    if(comparador){
                        MostrarTexto("RFID ya Guardado", "");
                    } else if (comparador == false){
                        topico = CrearCodigoTopico();
                        GuardarUsuarios(RFID, topico);
                    }
                } 
                flag = false;
                break;
            case 2:
                MenuSobrescribirUsuario();
                break;
            case 27:
                flag = false;
                break;
            default:
                MostrarTexto("Opcion Invalida", "");
                delay(3000);
                break;
            }
        }
    }
}

bool CompararRFID(){
    bool retorno = false;
    preferences.begin("Beniplas", false);   
    String Usuarios[] = {preferences.getString("Espacio1_RFID", String("")), preferences.getString("Espacio2_RFID", String("")), preferences.getString("Espacio3_RFID", String("")), preferences.getString("Espacio4_RFID", String("")), preferences.getString("Espacio5_RFID", String(""))};
    preferences.end();
    for(int i = 0; i < 5; i++){
        if(RFID == Usuarios[i]){
            retorno = true;
        }
    }
    return retorno;
}


String CrearCodigoTopico(){
    String asociacion = "EK";
    int aleatorio;
    for(int i = 0; i < 8; i++){
        aleatorio = random(0, 9);
        asociacion += (String)aleatorio;
    }
    MostrarTexto("El codigo  es:", asociacion);
    delay(20000);
    return asociacion;
}

void GuardarUsuarios(String RFID, String topico){
    int contador = ObtenerContador();
    char* nuevoRFID;
    char* nuevoTopico;
    char* espaciosRFID[] = {"", "Espacio1_RFID", "Espacio2_RFID", "Espacio3_RFID", "Espacio4_RFID", "Espacio5_RFID"};
    char* espaciosTopico[] = {"", "Espacio1_Topico", "Espacio2_Topico", "Espacio3_Topico", "Espacio4_Topico", "Espacio5_Topico"};
    if(contador >= 6){
        MostrarTexto("Limite de", "usuarios");
    } else{
        for(int i = 0; i < 6; i++){
            if(contador == i){
                nuevoRFID = espaciosRFID[i];
                nuevoTopico = espaciosTopico[i];
            }
        }
        preferences.begin("Beniplas", false);   
        preferences.putString(nuevoRFID, RFID);
        preferences.putString(nuevoTopico, topico);
        contador++;
        preferences.remove("contador");
        preferences.putInt("contador", contador);
        preferences.end();
    }
}

void MenuSobrescribirUsuario(){
    int contador = ObtenerContador() - 1;
    if(contador == 0){
        MostrarTexto("No se puede","sobrescribir");
        delay(3000);
    } else{
        String texto = (String)contador;
        texto += " ";
        lcd.clear();
        lcd.setCursor(0, 0);
        if(contador = 1){
            lcd.print(texto + " Persona"); 
            lcd.setCursor(0, 1);
            lcd.print("Registrada");
        } else {
            lcd.print(texto + " Personas"); 
            lcd.setCursor(0, 1);
            lcd.print("Registradas");
        }
        while(flag){
            if(wg.available() && wg.getCode() != 27){
                int espacio = wg.getCode();
                SobrescribirEspacio(espacio);
                flag = false;
            }
        }
    }
}

bool CompararRFIDSobrescribir(int lugar) { // Compara el RFID para  comprobar que sea igual al que se sobrescribira o diferente a los ya guardados
    bool retorno;
    int igual = 0;
    preferences.begin("Beniplas", false);
    String Espacios[] = {preferences.getString("Espacio1_RFID", String("")), preferences.getString("Espacio2_RFID", String("")), preferences.getString("Espacio3_RFID", String("")), preferences.getString("Espacio4_RFID", String("")), preferences.getString("Espacio5_RFID", String(""))};
    preferences.end();
    for(int i = 0; i < 5; i++){
        if(RFID == Espacios[i]){
            igual = i + 1;
        }
    }
    if(igual == lugar || igual == 0){
        retorno =  true;
    }
    if(igual != lugar && igual !=  0 ){
        retorno =  false;
    }
    return retorno;
}

void SobrescribirEspacio(int espacio){
    bool comparador;
    int contador = ObtenerContador() - 1;
    String topico;
    if(espacio > contador){
        MostrarTexto("Espacio Invalido", "");
        delay(3000);
    } else {
        SolicitarRFID();
        if(flag){
            comparador = CompararRFIDSobrescribir(espacio);
            preferences.begin("Beniplas", false);
            if(comparador){
                Serial.println("Hola");
                topico = CrearCodigoTopico();
                char* nuevoRFID;
                char* nuevoTopico;
                char* espaciosRFID[] = {"", "Espacio1_RFID", "Espacio2_RFID", "Espacio3_RFID", "Espacio4_RFID", "Espacio5_RFID"};
                char* espaciosTopico[] = {"", "Espacio1_Topico", "Espacio2_Topico", "Espacio3_Topico", "Espacio4_Topico", "Espacio5_Topico"};
                for(int i = 0; i < 6; i++){
                    if(espacio == i){
                        nuevoRFID = espaciosRFID[i];
                        nuevoTopico = espaciosTopico[i];
                    }
                }
                preferences.putString(nuevoRFID, RFID);
                preferences.putString(nuevoTopico, topico);
            }
            preferences.end();
            if(comparador == false){
                MostrarTexto("Debe ser igual o", "diferente");
                delay(3000);
            }
        } 
    }
}

void BloquearUsuario(){ //Función para bloquear a los usuarios
    tiempoBloqueo = 0;
    if(tiempoBloqueo == 0){
        tiempoBloqueo = millis();
    }  
    preferences.begin("Beniplas", false);
    preferences.putString("Bloqueo_1", RFID);
    preferences.end();
}

void RevisarBloqueos(){ //Función para contar el tiempo de bloqueo del usuario
    String topico;
    preferences.begin("Beniplas", false);
    String bloqueo = preferences.getString("Bloqueo_1", String(""));
    if(bloqueo != "" && millis() >= (tiempoBloqueo + 1800000)){
        topico = ObtenerTopico();
        preferences.remove("Bloqueo_1");
        EnviarNotificaciones(topico, "Tu RFID a sido desbloqueado");
    }
    preferences.end();
}

bool compararRFIDBloqueo(){ //Función para comparar si el RFID ingresado no esta bloqueado
    bool bandera;
    preferences.begin("Beniplas", false);
    String RFIDBloqueo = preferences.getString("Bloqueo_1", String(""));

    preferences.end();
    if(RFID == RFIDBloqueo){
        MostrarTexto("Tu RFID esta", "bloqueado");
        delay(3000);
        bandera = false;
    } else {
        bandera = true;
    }
    return bandera;
}

bool CompararCajasAbiertas(){ //Funcio para comprobar que cajas estan abiertas para que no se puedan volver abrir esas mismas
    bool abierto = true;
    preferences.begin("Beniplas", false);
    int CajasAbiertas[] = {preferences.getInt("1", 0), preferences.getInt("2", 0), preferences.getInt("3", 0), preferences.getInt("4", 0)};
    preferences.end();
    for(int i = 0; i < 4; i++){
        if(caja == CajasAbiertas[i]){
            MostrarTexto("La caja ya ha", "sido abierta");
            delay(2000);
            abierto = false;
        }
    }
    return abierto;
}

void EliminiarLista(){  //Eliminar la lista de las puertas abiertas
    preferences.begin("Beniplas", false);
    preferences.remove("1");
    preferences.remove("2");
    preferences.remove("3");
    preferences.remove("4");
    preferences.end();
}

void EscuchaFirebase(){ //Analizar la FireBase en tiempo real por si se quiere abrir el guarda valor desde el ordenador
    bool flagRealtime = false;
    bool iterar = true;
    bool cambio = false;
    //Leer puertas y cajas Izquierdas
    while(iterar){
        //leer puerta Izquierda
        Firebase.getBool(firebaseData, nodo + "/PuertaIzquierda");
        puertaI = firebaseData.boolData();
        //leer puerta Derecha
        Firebase.getBool(firebaseData, nodo + "/PuertaDerecha");
        puertaD = firebaseData.boolData();
        if(puertaI != false || puertaD != false){
            if(puertaI != false){
                opcion = 1;
                flagRealtime = true;
                AbrirPuerta();
            } else if(puertaD != false){
                opcion = 2;
                flagRealtime = true;
                AbrirPuerta();
                
            }
        }   
        iterar = false;
    }
    while(flagRealtime){
        Firebase.getBool(firebaseData, nodo + "/Caja1I");
        cajaI1 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja2I");
        cajaI2 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja3I");
        cajaI3 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja4I");
        cajaI4 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja1D");
        cajaD1 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja2D");
        cajaD2 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja3D");
        cajaD3 = firebaseData.boolData();
        Firebase.getBool(firebaseData, nodo + "/Caja4D");
        cajaD4 = firebaseData.boolData();
        if(cajaI1 != false){
            caja = 1;
            flagRealtime = false;
            cambio = true;
        } else if(cajaI2 != false){
            caja = 2;
            flagRealtime = false;
            cambio = true;
        } else if(cajaI3 != false){
            caja = 3;
            flagRealtime = false;
            cambio = true;
        } else if(cajaI4 != false){
            caja = 4;
            flagRealtime = false;
            cambio = true;
        } else if(cajaD1 != false){
            caja = 1;
            flagRealtime = false;
            cambio = true;
        } else if(cajaD2 != false){
            caja = 2;
            flagRealtime = false;
            cambio = true;
        } else if(cajaD3 != false){
            caja = 3;
            flagRealtime = false;
            cambio = true;
        } else if(cajaD4 != false){
            caja = 4;
            flagRealtime = false;
            cambio = true;
        }
    } 
    if(cambio)
    {
        AbrirCajas();
        ActualizarDatosFirebase();
        cambio = false;
    }   
}

void ActualizarDatosFirebase(){ // Actualizar los datos en la FireBase para dejarlos en falso y no dejar ninguna caja abierta
    Firebase.setBool(firebaseData, nodo + "/Caja1I", false);
    Firebase.setBool(firebaseData, nodo + "/Caja1D", false);
    Firebase.setBool(firebaseData, nodo + "/Caja2I", false);
    Firebase.setBool(firebaseData, nodo + "/Caja2D", false);
    Firebase.setBool(firebaseData, nodo + "/Caja3I", false);
    Firebase.setBool(firebaseData, nodo + "/Caja3D", false);
    Firebase.setBool(firebaseData, nodo + "/Caja4I", false);
    Firebase.setBool(firebaseData, nodo + "/Caja4D", false);
    Firebase.setBool(firebaseData, nodo + "/PuertaDerecha", false);
    Firebase.setBool(firebaseData, nodo + "/PuertaIzquierda", false);
}

void ConectarInternet(){
    lcd.clear();   
    lcd.setCursor(0, 0);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){ // Conectar el ESP32 a Internet
        delay(500);
        lcd.print(".");
    }
}