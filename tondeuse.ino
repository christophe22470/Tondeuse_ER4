#include <SoftwareSerial.h>
// lien avec le module bluetooth

#define RxD 2
#define TxD 3
//Arduino PWM Speed Control：
int E1 = 5;   
int M1 = 4; 
int E2 = 6;                       
int M2 = 7;
int CGAUCHE=9;
int CDROITE=8;
int vitesse=255;
 
#define GAUCHE 1
#define DROITE 2
#define DEVANT 3 
#define ARRIERE 4

SoftwareSerial BLE(RxD,TxD);

int etat;
#define EtatAVANCE 0
#define EtatRECULE 1
#define EtatTOURNE 2
#define EtatSTOP 3


unsigned long time;

//initialiser bluetooth shield par 
//des commandes AT
void setupBleConnection()
{
	BLE.begin(9600); 
	BLE.print("AT+CLEAR"); 
	//définir le rôle du module bluetooth étant esclave
	BLE.print("AT+ROLES"); 
	BLE.print("AT+SAVE1");
	delay(1000);
	//définir le nom du module
	BLE.print("AT+NAMEtondeuse"); 
	delay(1000);
	//définir le mot de passe du module bluetooth
	BLE.print("AT+PIN1234"); 
	delay(400);
	Serial.println("OK conf BT");
	BLE.println("BT OK");
}

void accelere(int sens, int v)
{
  	switch (sens)
  	{
		case DEVANT:
			digitalWrite(M1,HIGH);       
			digitalWrite(M2, HIGH);  
		break;
		case ARRIERE:
			digitalWrite(M1,LOW);       
			digitalWrite(M2, LOW);  
		break;
		case DROITE:
			digitalWrite(M1,LOW);       
			digitalWrite(M2, HIGH); 
		break;
		case GAUCHE:
			digitalWrite(M1,HIGH);       
			digitalWrite(M2, LOW);  
		break;
	}
    
	for (int i=50; i<=v; i++)
	{
		analogWrite(E1, i);   //PWM Speed Control   
		analogWrite(E2, i);   //PWM Speed Control   
		delay(10); 
	}
}

void decelere(int v)
{
	for (int i=v; i>=0; i--)
	{
		analogWrite(E1, i);   //PWM Speed Control   
		analogWrite(E2, i);   //PWM Speed Control   
		delay(10); 
	}
}


void arret()
{
   	digitalWrite(M1,LOW);       
    digitalWrite(M2, LOW);  
    analogWrite(E1, 0);   //PWM Speed Control   
    analogWrite(E2, 0);   //PWM Speed Control    
}  

int testImpact()
{
	int resu=0;
	if (digitalRead(CGAUCHE)==0) resu=1;
	if (digitalRead(CDROITE)==0) resu+=2;

	if (resu==1) Serial.println("Impact GAUCHE");
	if (resu==2) Serial.println("Impact DROIT");
	if (resu==3) Serial.println("Impact DEVANT");
	return resu;
}

void resetChrono()
{
  	time=millis();
}

unsigned long chrono()
{
	unsigned long resu =millis()-time;
	return resu;
}



void setup()  
{      
	pinMode(M1, OUTPUT);       
	pinMode(M2, OUTPUT); 
	pinMode(CGAUCHE, INPUT_PULLUP); 
	pinMode(CDROITE, INPUT_PULLUP); 
	Serial.begin(9600);
	setupBleConnection();
	
	arret();

	etat=EtatSTOP;
	accelere(DEVANT,0);
	resetChrono();
} 
int impa;

void loop() 
{
	int imp=testImpact();  
	if(BLE.available())
	{
		char c = BLE.read();
		Serial.println(c);
		if ((c>='0')&&(c<='9'))
		{
			vitesse=28*(c-'0');
			etat=EtatAVANCE;
			accelere(DEVANT,vitesse);
			resetChrono();
		}
		
		if (c=='a') 
		{
		etat=EtatAVANCE;
		accelere(DEVANT,vitesse);
		}
		if (c=='s') 
		{
		etat=EtatSTOP; 
		arret();
		}
		if (c=='g') 
		{
		etat=EtatSTOP; 
		arret();  accelere(GAUCHE,vitesse);
		}
		if (c=='d') 
		{
		etat=EtatSTOP; 
		arret();  accelere(DROITE,vitesse);
		}
		if (c=='r') 
		{
		etat=EtatRECULE;
		accelere(ARRIERE,vitesse);
		}
	}


    
	switch (etat)
	{
		case EtatAVANCE:
			if (chrono()>60000) 
			{  
				etat=EtatRECULE;
				arret();
				accelere(ARRIERE,vitesse);
				resetChrono();
			}
			if (imp!=0) 
			{  
				impa=imp;
				etat=EtatRECULE;
				resetChrono();
				arret();
				accelere(ARRIERE,255);
				
			}
			break;
		
		case EtatRECULE:
			if (chrono()>2000) 
			{ 
				if (impa==GAUCHE )
				{ etat=EtatTOURNE; arret();  accelere(DROITE,255); Serial.println(impa); }
				else 
				{ etat=EtatTOURNE; arret();  accelere(GAUCHE,255);Serial.println(impa); }
				resetChrono();
			}
		break;
		
		case EtatTOURNE:
			if (chrono()>200) 
			{  
				etat=EtatAVANCE;
				arret();
				accelere(DEVANT,vitesse);
				resetChrono();
			}
		break;
		
		
		break;
		}

}
