#include <LPC214x.H>                    /* LPC21xx definitions                */
#include <stdio.h>
#include "serial.h"


//#pragma import(__use_no_semihosting_swi)

extern int  sendchar(int ch);           /* Defined in Serial.c                */

struct __FILE { 
  int handle;                           /* Add whatever you need here         */ 
};

int fputc(int ch, FILE *f) {
  return (sendchar(ch));                /* Retarget fputc to serial UART      */
}

char hex2ascii(int);

void init_serial (void)  {              				/* Initialize Serial Interface        */
  PINSEL0 =  0x00000005;                				/* Enable RxD and TxD pins            */
  U0LCR = 0x83;                        				 	/* 8 bits, no Parity, 1 Stop bit      */
  
  //set 115200 baudrate for PCLK = FOSC / VPB DIV = 12 Mhz /4 = 3Mhz
  U0FDR = 0x85;
  U0DLL = 1; 						/* Setup Baudrate                 */
  U0DLM = 0;
  U0LCR = 0x03;                         				/* DLAB = 0                           */
}


/* implementation of putchar (also used by printf function to output data)    */
int sendchar (int ch)  {                /* Write character to Serial Port     */

  while (!(U0LSR & 0x20));

  return (U0THR = ch);
}


int getkey (void)  {                    /* Read character from Serial Port    */

  while (!(U0LSR & 0x01));

  return (U0RBR);
}


void sendascii (int ch) {

	int tmp;

	sendchar('0');
	sendchar('x');

	tmp = ch;
	tmp = tmp >> 4;	   		//select middle nibble
	tmp = hex2ascii(tmp);
	tmp = sendchar(tmp);    

	tmp = ch;
	tmp = hex2ascii(ch);   
	sendchar(tmp);
	
}

char hex2ascii(int nr) {

	char aux;
	aux = nr & 0x0F;   
	if (aux < 10) 
		aux = aux + '0';
	else aux = 	aux + '0' + 7;

	return aux;

}

void sendstring(char *a) {

	int i=0;

	
	while (a[i] != '\0') {
		sendchar(a[i]);
		i++;
	}

 		

}



