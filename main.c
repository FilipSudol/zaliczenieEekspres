/*
Testowy program do komunikacji BT z ekspresem
*/

#include <avr\io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>


//Nasze dwa przyciski
#define KEY_G1 (1<<PC0)
#define KEY_G2 (1<<PC1)

// inne zmienne potrzebne do dzialania programu
volatile unsigned char odpowiedz;
volatile unsigned char odb_flaga =0;
uint8_t key_lock, key_lock2;
volatile unsigned int usart_bufor_ind;
char usart_bufor[50] = "Dzien dobry, prosze wybrac kawe lub herbate";


//funkcja inicjujaca transmisje
void usart_inicjuj(void)
{
	//definicja parametrow transmisji
	#define BAUD 9600
	#include <util/setbaud.h>
	
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X
	UCSRA |=  (1<<U2X);
	#else
	UCSRA &= ~(1<<U2X);
	#endif
	
	//standardowe parametry transmisji modu�u HC-05
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);  //bit�w danych: 8  stopu: 1
	//w��cz nadajnik i odbiornik oraz ich przerwania odbiornika
	//przerwania nadajnika w��czamy w funkcji wyslij_wynik()
	UCSRB = (1<<TXEN) | (1<<RXEN) | (1<<RXCIE);
}


//przerwanie generowane po odebraniu bajtu
ISR(USART_RXC_vect)
{
	odpowiedz = UDR;   //zapami�taj odebran� liczb�
	odb_flaga = 1; //ustawiiamy flage
}



//przerwanie generowane, gdy bufor nadawania jest ju� pusty,
ISR(USART_UDRE_vect){
	
	//odpowiedzialne za wys�anie wszystkich znak�w z tablicy usart_bufor[]
	
	//sprawdzamy, czy bajt do wys�ania jest znakiem ko�ca tekstu, czyli zerem
	if(usart_bufor[usart_bufor_ind]!= 0){
		
		//za�aduj znak do rejestru wysy�ki i ustaw indeks na nast�pny znak
		UDR = usart_bufor[usart_bufor_ind++];
		
		}else{
		
		//osi�gni�to koniec napisu w tablicy usart_bufor[]
		UCSRB &= ~(1<<UDRIE); //wy��cz przerwania pustego bufora nadawania
	}
}


//--------------------------------------------------------------


void wyslij(void){
	
	//funkcja rozpoczyna wysy�anie, wysy�aj�c pierwszy znak znajduj�cy si�
	//w tablicy usart_bufor[]. Pozosta�e wy�le funkcja przerwania,
	//kt�ra zostanie wywo�ana automatycznie po wys�aniu ka�dego znaku.
	
	//dodaj do tekstu znaki ko�ca linii (CR+LF), by �adnie w terminalu wygladalo
	unsigned char z;
	for(z=0; z<50; z++){
		if(usart_bufor[z]==0){   //czy to koniec takstu w tablicy
			//tak znaleziono koniec tekstu, dodajemy znak CR
			usart_bufor[z]   = 13;  //znak powrotu karetki CR (Carrige Return)
			usart_bufor[z+1]  = 10; //znak nowej linii LF (Line Feed)
			usart_bufor[z+2]  = 0;  //znak ko�ca ci�gu tekstu w tablicy
			break;
		}
	}
	
	
	//Zaczekaj, a� bufor nadawania b�dzie pusty
	while (!(UCSRA & (1<<UDRE)));
	
	//bufor jest pusty mo�na wys�a�
	
	//nast�pny znak do wysy�ki to znak nr 1
	usart_bufor_ind = 0;
	
	//w��cz przerwania pustego bufora UDR, co rozpocznie transmisj�
	//aktualnej zawarto�ci bufora
	UCSRB |= (1<<UDRIE);
	
}

//--------------------------------------------------------------


int main(void)
{
	
	//................................./.INICJALIZACJA KLAWISZY................
	DDRC &= ~KEY_G1; // kierunek pinu PC0
	DDRC &= ~KEY_G2; // kierunek pinu PC1
	PORTC |= KEY_G1 | KEY_G2; // podci�gni�cie pin�w do VCC (wewn�trznym rezysotrem)
	//------------------------------
	usart_inicjuj();
	sei();
	wyslij();
	
	//**************************PETLA GLOWNA********************************
	while(1){
		

		//--------------Wybranie napoju zdlanie przez BT-------------------
		if(odb_flaga){
			
			odb_flaga = 0; //zga� flag�
			if(odpowiedz == 1)
			sprintf(usart_bufor, "%s", "Wybrano kawe, zycze smacznego");
			if(odpowiedz==2)
			sprintf(usart_bufor, "%s", "Wybrano herbate, zycze smacznego");
			
			wyslij();  //rozpocznij wysy�anie wyniku przez RS-232
		}
		//------------------Wybranie napoju manualnie przez przyciski--------------------------------
		//Sprawdza wci�ni�cie klawisza herbaty
		if( !key_lock && !(PINC & KEY_G1 ) ) key_lock=1;
		else if( key_lock && (PINC & KEY_G1 ) ) {
			if( !++key_lock ) {
				
				// reakcja na PUSH_UP (zwolnienie przycisku)
				sprintf(usart_bufor, "%s", "Wybrano herbate, zycze smacznego");
				wyslij();
			}
		}
		//Sprawdza wci�ni�cie klawisza kawy
		if( !key_lock2 && !(PINC & KEY_G2 ) ) key_lock2=1;
		else if( key_lock2 && (PINC & KEY_G2 ) ) {
			if( !++key_lock2 ) {
				
				// reakcja na PUSH_UP (zwolnienie przycisku)
				sprintf(usart_bufor, "%s", "Wybrano kawe, zycze smacznego");
				wyslij();
			}
		}
		
		
	}
}