#include <SoftwareSerial.h>
#include <MsTimer2.h>

//Lien avec le module bluetooth
#define RxD 2
#define TxD 3

//Paramètres moteur coupe
#define Km 0.3
#define Kmv 0.4
#define PDDMoteur 0.5
#define pinTensionMoteur A0
#define pinMoteurCoupe 8
double consigneVitesse = 0.0;

//Paramètres PI
#define Ki 0.6
#define Kp 0.5

//Paramètre alimentation
#define tensionBatterieMax 12
#define pinTensionBatterie A1
#define PDDBatterie 0.5
double tensionBatterie = 12.0;

//Parametres moteur lateraux
#define sensMoteur1 5 //pin Speed moteur 1
#define powerMoteur1 4 //pin sens moteur 1
#define sensMoteur2 6 //pin Speed moteur 2   
#define powerMoteur2 7 //pin sens moteur 2

#define vitesseRotation 5 // deg/ms
#define vitesseDeplacement 50 //entre 0 et 255
int consigneVitesseDeplacement = 50;

//Paramètres détection d'impacte
#define pinImpactDroit 2 //Pin impacte droit
#define pinImpactGauche 3 //Pin impacte gauche

//Paramètres detection de périmetre
#define pinDetectPerim 9 //Pin détection de périmètre

int MODE = 0;
//0 -> non renseigné
//1 -> mode autonome
//2 -> mode piloté

bool autoIsStarted = false; //Le mode auto est démarré

SoftwareSerial BLE(RxD,TxD); //Init bluetooth


//initialiser bluetooth shield par des commandes AT
void setupBleConnection(){

	BLE.begin(9600); 
	BLE.print("AT+CLEAR"); 
	//définir le rôle du module bluetooth étant esclave
	BLE.print("AT+ROLES"); 
	BLE.print("AT+SAVE1");
	delay(1000);
	//définir le nom du module
	BLE.print("AT+NAMEtondeuseAuto"); 
	delay(1000);
	//définir le mot de passe du module bluetooth
	BLE.print("AT+PIN0000"); 
	delay(400);
	Serial.println("OK conf BT");
	BLE.println("BT OK");
}

//stopper le robot
void stop(){
   	digitalWrite(sensMoteur1,LOW); //sens du moteur 1 en arriere
    digitalWrite(sensMoteur2, LOW); //sens du moteur 2 en arriere
    analogWrite(powerMoteur1, 0); //puissance du moteur 1 à 0
    analogWrite(powerMoteur2, 0); //puissance du moteur 2 à 0
}

//pivoter à droite
void right(int degre){

	unsigned long start = millis(); // temps début
	long dTime = degre/vitesseRotation; //en ms

	while(millis() < start+dTime){
		digitalWrite(sensMoteur1, LOW); //sens du moteur 1 en avant
		digitalWrite(sensMoteur2, HIGH); //sens du moteur 2 en avant
		analogWrite(powerMoteur1, 50); //puissance du moteur 1 à la consigne
		analogWrite(powerMoteur2, 50); //puissance du moteur 2 à la consigne
	}

	stop(); //stopper la rotation
}

//pivoter à gauche
void left(int degre){

	unsigned long start = millis(); // temps début
	long dTime = degre/vitesseRotation; //en ms

	while(millis() < start+dTime){
		digitalWrite(sensMoteur1, HIGH); //sens du moteur 1 en avant
		digitalWrite(sensMoteur2, LOW); //sens du moteur 2 en avant
		analogWrite(powerMoteur1, 50); //puissance du moteur 1 à la consigne
		analogWrite(powerMoteur2, 50); //puissance du moteur 2 à la consigne
	}

	stop(); //stopper la rotation
}

//avancer
void front(int vitesse){
	digitalWrite(sensMoteur1, HIGH); //sens du moteur 1 en avant
    digitalWrite(sensMoteur2, HIGH); //sens du moteur 2 en avant
    analogWrite(powerMoteur1, vitesse); //puissance du moteur 1 à la consigne
    analogWrite(powerMoteur2, vitesse); //puissance du moteur 2 à la consigne
}

//reculer
void back(int vitesse){
	digitalWrite(sensMoteur1,LOW); //sens du moteur 1 en arriere
    digitalWrite(sensMoteur2, LOW); //sens du moteur 2 en arriere
    analogWrite(powerMoteur1, vitesse); //puissance du moteur 1 à la consigne
    analogWrite(powerMoteur2, vitesse); //puissance du moteur 2 à la consigne
}

void setup()  
{      
	pinMode(pinTensionMoteur, INPUT); //Setup pin tension moteur de coupe
	pinMode(pinMoteurCoupe, OUTPUT); //Setup pin commande moteur de coupe

	pinMode(pinTensionBatterie, INPUT); //Setup pin tension batterie

	pinMode(sensMoteur1, OUTPUT); //Setup pin sens moteur 1
	pinMode(sensMoteur2, OUTPUT); //Setup pin sens moteur 2
	pinMode(powerMoteur1, OUTPUT); //Setup pin power moteur 1
	pinMode(powerMoteur2, OUTPUT); //Setup pin power moteur 2

	pinMode(pinImpactDroit, INPUT_PULLUP);
	pinMode(pinImpactGauche, INPUT_PULLUP);

	stop(); //Stopper le moteur à la mise sous tension
	CommandeMoteurCoupe(0);

	Serial.begin(9600); //Setup port serie
	setupBleConnection(); //Setup bluetooth

	//interruptions
	MsTimer2::set(200, istrCommandeMoteurCoupe); //Interruption moteur coupe toutes les 200ms 
	MsTimer2::start(); //Activer le timer 2
}

void loop() 
{
	int impactDroit = digitalRead(pinImpactDroit);
	int impactGauche = digitalRead(pinImpactGauche);
	int detectPerim = digitalRead(pinDetectPerim);

	if(impactGauche && impactDroit){
		stop();
		back(vitesseDeplacement);
		delay(500);
		right(90);
		front(vitesseDeplacement);
	}else if(impactGauche){
		stop();
		back(vitesseDeplacement);
		delay(500);
		right(30);
		front(vitesseDeplacement);
	}else if(impactDroit){
		stop();
		back(vitesseDeplacement);
		delay(500);
		left(30);
		front(vitesseDeplacement);
	}

	if(detectPerim){
		stop();
		back(vitesseDeplacement);
		delay(200);
		left(90);
		front(vitesseDeplacement);
	}

	if(BLE.available())
	{
		char c = BLE.read();
		Serial.println(c);
		if ((c>='0')&&(c<='9')){
			consigneVitesseDeplacement = map(c-'0', 0, 10, 0, 255);
		}else if(c=='a'){
			MODE = 1; //Mode automatique
		}else if(c=='p'){
			MODE = 2; //Mode piloté
			autoIsStarted = false;
		}else if(c=='d' && MODE==1 && !autoIsStarted){ //Démarrage en mode auto
			front(vitesseDeplacement); //Démarrer le robot vers l'avant
			autoIsStarted = true; //Le mode auto est démarré
		}else if(c=='f' && MODE==2){
			front(consigneVitesseDeplacement); //Déplacement vers l'avant
		}else if(c=='b' && MODE==2){
			back(consigneVitesseDeplacement); //Déplacement vers l'arrière
		}else if(c=='r' && MODE==2){
			right(30); //Pivot de 30 degrés à droite
		}else if(c=='l' && MODE==2){
			left(30); //Pivot de 30 degrés à gauche
		}else if(c=='s'){
			stop(); //Stopper le robot
		}
	}
}

//probleme timer et variables volatiles
void istrCommandeMoteurCoupe(){

	//Couper la tension aux bornes du moteur
	CommandeMoteurCoupe(0);
	
	static unsigned long lastInterrup = 0.0; //sauvegarde temps derniere interruption
	unsigned long now = millis(); //sauvegarder temps actuel
	unsigned long dt = lastInterrup - now; //calcule dt entre 2 interruption
	static double I_x = 0.0; //integrale

	//Calcule de la tension du moteur
  	double Vvitesse = (analogRead(pinTensionMoteur) * PDDMoteur)/Kmv;

  	/******* Régulation PID ********/
  	// Ecart entre la tension de la consigne et la mesure
  	double ecart = Kmv * consigneVitesse - Vvitesse;
 
  	// Calcul de la commande
  	double commande = (Kp * ecart) + I_x;

	//Terme intégral (sera utilisé lors du pas d'échantillonnage suivant)
	I_x = I_x + Ki * dt * ecart;
	/******* Fin régulation PID ********/

	lastInterrup = now; //sauvegarde temps derniere interruption
	CommandeMoteurCoupe(commande); //envoi de la commande au moteur

	tensionBatterie = analogRead(pinTensionBatterie) * PDDBatterie; //acquisition de la tension de la batterie
}

void CommandeMoteurCoupe(double tensionMoy){

	//eviter les saturations
	if(tensionMoy<0){
		tensionMoy=0;
	}else if(tensionMoy>tensionBatterie){
		tensionMoy = tensionBatterie;
	}

	//envoi de la commande au moteur en PWM
	digitalWrite(pinMoteurCoupe, map(tensionMoy, 0, tensionBatterie, 0, 255));
}
