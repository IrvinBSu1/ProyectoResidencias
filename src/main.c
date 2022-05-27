#include <stdio.h>
#include <pic18f45k50.h>
#include "support.h"

#pragma config XINST = OFF

#define svnum 3              //Define la cantidad de servos a controlar
#define svslot (65536UL - (2000UL/svnum)) //Calculo del slot de tiempo a cada servo
#define sv1pin PORTAbits.RA0 //Pin del servo1
#define sv2pin PORTAbits.RA1 //Pin del servo2
#define sv3pin PORTAbits.RA2 //Pin del servo3
#define bt1pin PORTBbits.RB0 //Pulsador 1
#define bt2pin PORTBbits.RB1 //Pulsador 2
#define bt3pin PORTBbits.RB2 //Pulsador 3
#define left 900             //Definicion de tiempo para posicion 0° (900ms)
#define center 1400          //Definicion de tiempo para posicion 90° (1400ms)
#define right 1900           //Definicion de tiempo para posicion 180° (1900ms)

// LCD Constants
#define LCD_DATA PORTB
#define LCD_DATA_TRIS TRISB
#define LCD_ENABLE PORTCbits.RC0
#define LCD_RW PORTCbits.RC1
#define LCD_RS PORTCbits.RC2

//Entry mode variables
#define INC_CURSOR 0x02
#define DEC_CURSOR 0x00
#define SHIFT_ON 0x01

//Display control variables
#define DISPLAY_ON 0x04
#define DISPLAY_OFF 0x00
#define CURSOR_ON 0x02
#define BLINK_ON 0x01

//Cursor move variables
#define SHIFT_DISP 0x08
#define SHIFT_RIGHT 0x04
#define SHIFT_LEFT 0x00

//Function set variables
#define IFACE_4BIT 0x00
#define IFACE_8BIT 0x10
#define DUAL_LINE 0x08
#define DOTS_5X11 0x04

//DDRAM locations
#define FIRST_LINE 0x00
#define SECOND_LINE 0x40

//static __code char __at(__CONFIG1H) config1h = 0xFF & _FOSC_INTOSCIO_1H;
//static __code char __at(__CONFIG2L) config2l = 0xFF & _nPWRTEN_ON_2L;
//static __code char __at(__CONFIG2H) config2h = 0xFF & _WDTEN_OFF_2H;
//static __code char __at(__CONFIG4L) config4l = 0xFF & _XINST_OFF_4L;

volatile unsigned int svtime, sv1val = center, sv2val = center, sv3val = center;
unsigned int itemCounter; // Contador para determinar el numero de items
char svon = 0, ledcnt = 0;
char bt1val, bt2val, bt3val; //Variables de estado de pulsadores
char bt1cnt, bt2cnt, bt3cnt; //Contadores de tiempo para los pulsadores
char bt1ok = 0, bt2ok = 0, bt3ok = 0; //Banderas de confirmación de pulsadores

//Prototypes
void lcd_init(void);

void lcd_clear(void);
void lcd_home(void);
void lcd_emode(unsigned char);
void lcd_dmode(unsigned char);
void lcd_cmode(unsigned char);
void lcd_fmode(unsigned char);
void lcd_ddram(unsigned char);

void send_cmd(unsigned char);
void send_data(unsigned char);

void lcd_putrs(const char *a);

char lcd_busy(void);

#pragma code isr 0x2008
void isr()   //Rutina de servicio a la interrupción
{
    // Deshabilitar interrupciones si es necesario
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
    OSCCONbits.IRCF = 0b100; // Habilitamos el oscilador interno a 6Mhz
    ANSELA = 0;  //Configura los pines AN0-AN7 en modo digital
    ANSELB = 0;  //Configura los pines AN8-AN15 en modo digital
    ANSELC = 0;  //Configura los pines del PORTC en modo digital
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

    //LCD lcd = { &PORTC, 2, 3, 4, 5, 6, 7 }; // PORT, RS, EN, D4, D5, D6, D7
    lcd_init();

    while(1)
    {
        //LCD_Clear();
        //LCD_Set_Cursor(1,0); // tenemos que investigar los comandos
        //LCD_putrs("  HELLO WORLD!  "); // La cantidad máxima de caracteres por linea es 16
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
        // Para un retardo de 1 segundo necesitamos 6 millones de ciclos
        delay_ms(20);
    }
}

void main(void)
{
    // Funcion principal
    mainCode();
}






















/**
 * lcd_init(void) - Initialise Display
 */
void lcd_init(void)
{

    unsigned int loop;

    // Set data bus as output
    LCD_DATA_TRIS = 0x00;
    // Initialise data bus
    LCD_DATA = 0x00;

    // Set control lines as output
    TRISC = 0xF0;

    // Set all conrol lines to 0;
    LCD_ENABLE = 0;
    LCD_RW = 0;
    LCD_RS = 0;

    // Wait for more than 30ms
    loop = 0;
    while(loop < 9000){
        loop++;
    }

    // Clear LCD
    lcd_clear();

    // Set entry mode
    lcd_emode(INC_CURSOR);

    // Select Function Set
    lcd_fmode(IFACE_8BIT | DUAL_LINE);

    // Configure display mode
    lcd_dmode(DISPLAY_ON);
}

/**
 * lcd_clear() - Clear the LCD Display and return cursor to home position
 *
 */
void lcd_clear(void)
{
    // Send the command
    send_cmd(0x01);

}

/**
 * lcd_home() - Return cursor to home position (contents of diaplay unchanged)
 *
 */
void lcd_home(void)
{
    // Send the command
    send_cmd(0x02);
}

/**
 * set_emode(options) - Set entry mode
 *
 * Options:
 *
 *  INC_CURSOR - Incremnt cursor after character written
 *  DEC_CURSOR - Decrement cursor after character written
 *  SHIFT_ON - Switch Cursor shifting on
 */
void lcd_emode(unsigned char options)
{
    // Ensure that only valid range of bits set
    options = options & 0x03;
    // Set display command bit
    options = options | 0x04;
    // Send the command
    send_cmd(options);
}

/**
 * set_dmode(options) - Configure display mode
 *
 * Options:
 *
 *  DISPLAY_ON - Turn Display on
 *  DISPLAY_OFF - Turn Display off
 *  CURSOR_ON  - Turn Cursor on
 *  BLINK_ON - Blink Cursor
 */
void lcd_dmode(unsigned char options)
{
    // Ensure that only valid range of bits set
    options = options & 0x07;
    // Set display command bit
    options = options | 0x08;
    // Send the command
    send_cmd(options);
}

/**
 * set_cmode(options) - Configure cursor mode
 *
 * Options:
 *
 *  SHIFT_DISP - Shift Display
 *  SHIFT_RIGHT - Move cursor right
 *  SHIFT_LEFT - Move cursor left
 */
void lcd_cmode(unsigned char options)
{
    // Ensure that only valid range of bits set
    options = options & 0x0C;
    // Set display command bit
    options = options | 0x10;
    // Send the command
    send_cmd(options);
}

/**
 * set_fmode(options) - Configure function set
 *
 * Options:
 *
 *  4BIT_IFACE - 4-bit interface
 *  8BIT_IFACE - 8-bit interface
 *  1_16_DUTY - 1/16 duty
 *  5X10_DOTS - 5x10 dot characters
 */
void lcd_fmode(unsigned char options)
{
    // Ensure that only valid range of bits set
    options = options & 0x1F;
    // Set display command bit
    options = options | 0x20;
    // Send the command
    send_cmd(options);
}

/**
 * set_ddram(address) - Set DDRAM address
 *
 * Options:
 *
 *  address - 7-bit address
 */
void lcd_ddram(unsigned char address)
{
    // Ensure that only valid range of bits set
    address = address & 0x7F;
    // Set DDRAM bit
    address = address | 0x80;
    // Send the command
    send_cmd(address);
}

/**
 * send_cmd(unsigned char) - Send command
 *
 */
void send_cmd(unsigned char command)
{
    int loop;

    // Wait for LCD to be ready
    while(lcd_busy() == 1);

    // Set data bus as output
    LCD_DATA_TRIS = 0x00;
    // Set directon control line to write
    LCD_RW = 0;
    // Clear command/data flag (command)
    LCD_RS = 0;
    //Put command on data bus
    LCD_DATA = command;
    // Raise Enable Line
    LCD_ENABLE = 1;
    //NOTE: Enable Lime must be up for atleast 450ns!
    //NOTE: Data must be valid after 320ns from LCD_ENABLE rising

    //Short pause
    loop = 0;
    while(loop < 20){
        loop++;
    }
    // Lower Enable Line
    LCD_ENABLE = 0;
}

/**
 * send_data(unsigned char) - Send data
 *
 */
void send_data(unsigned char dataval)
{
    int loop;

    // Wait for LCD to be ready
    while(lcd_busy() == 1);

    // Set data bus as output
    LCD_DATA_TRIS = 0x00;
    // Set directon control line to write
    LCD_RW = 0;
    // Set command/data flag (data)
    LCD_RS = 1;
    //Put command on data bus
    LCD_DATA = dataval;
    // Raise Enable Line
    LCD_ENABLE = 1;
    //NOTE: Enable Lime must be up for atleast 450ns!
    //NOTE: Data must be valid after 320ns from LCD_ENABLE rising

    //Short pause
    loop = 0;
    while(loop < 20){
        loop++;
    }
    // Lower Enable Line
    LCD_ENABLE = 0;
}

/**
 * lcd_busy(void) - Check to see if LCD is ready
 *
 * Return:
 *  0 - Ready
 *  1 - Busy
 */
char lcd_busy(void)
{
    int loop;
    unsigned char dataval;

    // Set data bus as input
    LCD_DATA_TRIS = 0xFF;
    // Initialise data bus
    LCD_DATA = 0x00;
    // Set directon control line to write
    LCD_RW = 1;
    // Set command/data flag (command)
    LCD_RS = 0;
    // Raise Enable Line
    LCD_ENABLE = 1;
    //NOTE: Enable Lime must be up for atleast 450ns!
    //NOTE: Data must be valid after 320ns from LCD_ENABLE rising
    //Short pause
    loop = 0;
    while(loop < 20){
        loop++;
    }
    // Read data bus
    dataval = LCD_DATA;
    // Lower Enable Line
    LCD_ENABLE = 0;

    // Mask all but busy flag
    dataval = dataval & 0x80;
    if(dataval == 0x80){
        // Set data bus as output
        LCD_DATA_TRIS = 0x00;
        return(1);
    }
    else
    {
        // Set data bus as output
        LCD_DATA_TRIS = 0x00;
        return(0);
    }

}

void lcd_putrs ( const char *a ) {

    for ( int i = 0; a[i] != '\0'; ++i ) {
        send_data(a[i]);
    }

}

