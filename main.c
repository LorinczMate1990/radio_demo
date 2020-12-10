#include "hardware_board.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "timer.h"

volatile char packetReceivedOrSent = 0;
volatile char packetSent = 0;

// TODO Ez a függvény nem jó
void printNumber(char *label, unsigned char value) {
    RingBuffer_putString(&txUartBuffer, label);
    RingBuffer_putString(&txUartBuffer, ": ");

    for (char i=100; i>0; i /= 10) {
        RingBuffer_putChar(&txUartBuffer, (value / i) + '0' );
        value %= i;
    }

    RingBuffer_putString(&txUartBuffer, "\n\r");
}

int main(){
    initBoard(ENABLE_BUTTON_INTERRUPT | ENABLE_SERIAL_PORT | ENABLE_CC1101);

    enableGlobalInterrupt();
    RingBuffer_putString(&txUartBuffer, "Start program \x30 \n\r");
    
    initCC1101(); // Ehhez kellenek megszakítások
    RingBuffer_putString(&txUartBuffer, "initCC1101 finished\n\r\n\r");
    enableRisingEdgeInterrupt(&CC1101_GD0);

    setOutputLow(&GREEN_LED);
    setOutputLow(&RED_LED);

    __delay_cycles(16000000);

    setOutputHigh(&GREEN_LED);

    while(1) {
        __delay_cycles(160000);

        // Sorosport kezelése
        if (RingBuffer_getCharNumber(&rxUartBuffer) >= 1) {
            char rec = RingBuffer_getChar(&rxUartBuffer);
            RingBuffer_putChar(&txUartBuffer, rec);
            if (rec == 's' && packetSent != 1 ) {
                writeIntoTXBuffer("\x06radio", 7);
                setTXMode();
                RingBuffer_putString(&txUartBuffer, "Kuld\n\r");
                packetSent = 1;
            }
        }

        if (packetReceivedOrSent) {
            if (packetSent) {
                packetSent = 0;
                RingBuffer_putString(&txUartBuffer, "Kuld kesz\n\r");
            } else {
                RingBuffer_putString(&txUartBuffer, "Fogad: ");
                unsigned char len = readRFReg(TI_CC_REG_RXFIFO);
                const char maxLength = 50;
                char *buffer[maxLength];
                if (len+2 < maxLength){
                    burstReadRFReg(TI_CC_REG_RXFIFO, buffer, len+2); // RSSI és LQI miatt
                } else {
                    // Nem tudom kiolvasni, de a buffert ki kell üríteni.
                    RingBuffer_putString(&txUartBuffer, "\n\rOF\n\r");
                    dumpBurstReadRFReg(TI_CC_REG_RXFIFO, len+2);
                }

                const char rssi = buffer[len];
                const char lqi = buffer[len+1];

                buffer[len] = 0;
                printNumber("rssi", rssi);
                printNumber("lqi ", lqi);

                RingBuffer_putString(&txUartBuffer, buffer);
                RingBuffer_putString(&txUartBuffer, "\n\r");
            }
            packetReceivedOrSent = 0;
        }
    }
}

__attribute__((interrupt(PORT2_VECTOR)))
void CC1101Interrupt(){
    if (testInterruptFlag(&CC1101_GD0)){

        packetReceivedOrSent = 1; // Ez bejön küldés után is!!!

        clearInterruptFlag(&CC1101_GD0);
    }
}

__attribute__((interrupt(PORT1_VECTOR)))
void buttonInterrupt(){
    if (testInterruptFlag(&BUTTON)){
        toggleOutput(&RED_LED);
        clearInterruptFlag(&BUTTON);   
    }
}