#include <delay.h>

#define bool int
#define true  1
#define false 0

int bit_test(unsigned bit){
    return bit == 1;
}

// No usar para delays de 1 milisegundo
void delay_ms(int milis){
    /*
 * the delayNNtcy family of functions performs a
 * delay of NN cycles. Possible values for NN are:
 *   10  10*n cycles delay
 *  100  100*n cycles delay
 *   1k  1000*n cycles delay
 *  10k  10000*n cycles delay
 * 100k  100000*n cycles delay
 *   1m  1000000*n cycles delay
 */
    // Suponiendo que la frecuencia base es de 6MHz
    // N = n * (FOSC/4)
    // N => Numero de ciclos requeridos
    // n => Segundos del delay
    // FOSC => Frecuencia del reloj
    int required = ((milis / 1000) * (1500000)) / 1000; // 1 millon y medio equivale a una frecuencia de 4 MHz
    delay1ktcy(required);
}

void delay_us(int micros){
    /*
 * the delayNNtcy family of functions performs a
 * delay of NN cycles. Possible values for NN are:
 *   10  10*n cycles delay
 *  100  100*n cycles delay
 *   1k  1000*n cycles delay
 *  10k  10000*n cycles delay
 * 100k  100000*n cycles delay
 *   1m  1000000*n cycles delay
 */
    // Suponiendo que la frecuencia base es de 6MHz
    // N = n * (FOSC/4)
    // N => Numero de ciclos requeridos
    // n => Segundos del delay
    // FOSC => Frecuencia del reloj
    int required = ((micros / 1000000) * (1500000)) / 10; // 1 millon y medio equivale a una frecuencia de 4 MHz
    delay10tcy(required);
}