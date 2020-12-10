#include "hardware_board.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "timer.h"

volatile char packetReceivedOrSent = 0;
volatile char packetSent = 0;

int main(){
    initBoard(ENABLE_BUTTON_INTERRUPT | ENABLE_SERIAL_PORT | ENABLE_CC1101);

    enableGlobalInterrupt();
    RingBuffer_putString(&txUartBuffer, "Start program\n\r");
    
    initCC1101(); // Ehhez kellenek megszakítások
    RingBuffer_putString(&txUartBuffer, "initCC1101 finished\n\r\n\r");
    enableRisingEdgeInterrupt(&CC1101_GD0);

    setOutputLow(&GREEN_LED);

    __delay_cycles(16000000);

    setOutputHigh(&GREEN_LED);
    setOutputLow(&RED_LED);

    while(1) {
        //RingBuffer_putString(&txUartBuffer, "\n\r___\n\r");
        //RingBuffer_putChar(&txUartBuffer, packetReceivedOrSent+0x30);
        //RingBuffer_putChar(&txUartBuffer, packetSent+0x30);
        __delay_cycles(160000);


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
                unsigned char len = readRFReg(0x3f);
                const char maxLength = 50;
                char *buffer[maxLength];
                if (len+2<maxLength){
                    burstReadRFReg(0x3f, buffer, len+2); // RSSI és LQI miatt
                } else {
                    // Nem tudom kiolvasni, de a buffert ki kell üríteni.
                    RingBuffer_putString(&txUartBuffer, "\n\rOF\n\r");
                    dumpBurstReadRFReg(0x3f, len+2);
                }
                const rssi = buffer[len];
                const lqi = buffer[len+1];
                buffer[len] = 0;
                RingBuffer_putString(&txUartBuffer, buffer);
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