#include <stdio.h>
#include <pic18f45k50.h>
#include "support.h"
//#pragma config FOSC = INTRC_NOCLKOUT, WDTE = OFF, LVP = OFF
//#include <xc.h>
//#define _XTAL_FREQ 4_000_000
#pragma config XINST = OFF
#define svnum 3 //Define la cantidad de servos a controlar
#define svslot (65536UL - (2000UL/svnum)) //Calculo del slot de tiempo a cada servo
#define sv1pin PORTAbits.RA0 //Pin del servo1
#define sv2pin PORTAbits.RA1 //Pin del servo2
#define sv3pin PORTAbits.RA2 //Pin del servo3
#define bt1pin PORTBbits.RB0 //Pulsador 1
#define bt2pin PORTBbits.RB1 //Pulsador 2
#define bt3pin PORTBbits.RB2 //Pulsador 3
#define left 900   //Definicion de tiempo para posicion 0° (900ms)
#define center 1400  //Definicion de tiempo para posicion 90° (1400ms)
#define right 1900  //Definicion de tiempo para posicion 180° (1900ms)

volatile unsigned int svtime, sv1val = center, sv2val = center, sv3val = center;
char svon = 0, ledcnt = 0;
char bt1val, bt2val, bt3val; //Variables de estado de pulsadores
char bt1cnt, bt2cnt, bt3cnt; //Contadores de tiempo para los pulsadores
char bt1ok = 0, bt2ok = 0, bt3ok = 0; //Banderas de confirmación de pulsadores

#pragma code isr 0x2008
void isr()    //Rutina de servicio a la interrupción
{
    if(PIR1bits.TMR1IF == 1) //Condición de desbordamiento con tiempo variable
    {
        switch(svon)
        {
            case 0:
                sv1pin = !sv1pin; //Genera pulso de control servo1
                if(sv1pin)
                    svtime = 65536 - sv1val; //Tiempo para finalizar pulso
                else
                {
                    svtime = svslot - sv1val; //Tiempo para iniciar pulso
                    svon ++;
                }
                break;
            case 1:
                sv2pin = !sv2pin; //Genera pulso de control servo2
                if(sv2pin)
                    svtime = 65536 - sv2val; //Tiempo para finalizar pulso
                else
                {
                    svtime = svslot - sv2val; //Tiempo para iniciar pulso
                    svon ++;
                }
                break;
            case 2:
                sv3pin = !sv3pin; //Genera pulso de control servo2
                if(sv3pin)
                    svtime = 65536 - sv3val; //Tiempo para finalizar pulso
                else
                {
                    svtime = svslot - sv3val; //Tiempo para iniciar pulso
                    svon ++;
                }
                break;
        }
        if(svon >= svnum) //Si el control supera al máximo de servos
            svon = 0;
        T1CONbits.TMR1ON = 0; //Para el Modulo TMR1
        TMR1H = svtime >> 8;
        TMR1L = svtime;    //Actualiza el registro contador
        T1CONbits.TMR1ON = 1; //Arranca el contador
        PIR1bits.TMR1IF = 0; //Limpia la bandera

    }
}

void mainCode(void){
    ANSELA = 0;  //Configura los pines AN0-AN7 en modo digital
    ANSELB = 0; //Configura los pines AN8-AN15 en modo digital
    TRISB = 0b00000111; //Pulsadores en RB0,RB1,RB2
    WPUB = 0b11111111; //Acvita las resistencias PullUP
    TRISA = 0;  //Configura como salida todos los pines del PORTA
    PORTA = 0;  //Coloca en 0 lógico los pines del PORTA
    TRISEbits.TRISE2 = 0; //Configura el pin RE2 como salida
    T1CONbits.TMR1CS = 0; //Ajusta el modo Temporizador
    T1CONbits.T1CKPS = 0b00; //Sin pre-escala o 1:1
    TMR1H = 0;
    TMR1L = 0;
    PIR1bits.TMR1IF = 0; //Limpia la bandera
    INTCONbits.PEIE = 1; //Cada pulso capturado es de 1uS (4*Tosc)
    PIE1bits.TMR1IE = 1; //Activa la interrupción del TMR1
    INTCONbits.GIE = 1; //Habilitador Global
    T1CONbits.TMR1ON = 1; //Arranca el modulo TMR1

    while(1)
    {
        if(bt1pin != bt1val)
        {
            bt1cnt ++;
            if(bt1cnt > 200) //El pulsador1 debe estar presionado por lo
            {     //menos unos 200ms para confirmar
                bt1cnt = 0;
                bt1val = !bt1val;
                bt1ok = 1; //Confirma que pulsador1 fue presionado
            }
        }
        else bt1cnt = 0; //Reinicia contador cuando el pulsador1 cambia
        if(bt2pin != bt2val)
        {
            bt2cnt ++;
            if(bt2cnt > 200)//El pulsador2 debe estar presionado por lo
            {     //menos unos 200ms para confirmar
                bt2cnt = 0;
                bt2val = !bt2val;
                bt2ok = 1; //Confirma que pulsador2 fue presionado
            }
        }
        else bt2cnt = 0; //Reinicia contador cuando el pulsador2 cambia

        if(bt3pin != bt3val)
        {
            bt3cnt ++;
            if(bt3cnt > 200)//El pulsador3 debe estar presionado por lo
            {     //menos unos 200ms para confirmar
                bt3cnt = 0;
                bt3val = !bt3val;
                bt3ok = 1; //Confirma que pulsador3 fue presionado
            }
        }
        else bt3cnt = 0; //Reinicia contador cuando el pulsador3 cambia
        if(bt1ok) //Verifica si el pulsador1 fue presionado y confirmado
        {
            if(bt1val == 1)
                sv1val = left;
            else
                sv1val = right;
            bt1ok = 0;
        }
        if(bt2ok) //Verifica si el pulsador2 fue presionado y confirmado
        {
            if(bt2val == 1)
                sv2val = left;
            else
                sv2val = right;
            bt2ok = 0;
        }
        if(bt3ok) //Verifica si el pulsador3 fue presionado y confirmado
        {
            if(bt3val == 1)
                sv3val = left;
            else
                sv3val = right;
            bt3ok = 0;
        }
        if(ledcnt++ > 100) //Aproximado 100x1ms = 100ms
        {
            ledcnt = 0; //Reinicia contador de ciclos (1ms)
            PORTEbits.RE2 = !PORTEbits.RE2; //Destella LED conectado a RE2
        }
        // Necesita Calculo
        // Para un retardo de 1 segundo necesitamos 4 millones de ciclos
        delay_ms(1);
    }
}

void main(void) //Funcion principal
{
    mainCode();
}