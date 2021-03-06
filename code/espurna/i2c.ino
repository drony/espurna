/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if I2C_SUPPORT

#include "brzo_i2c.h"

int _i2cClearbus(int sda, int scl) {

    #if defined(TWCR) && defined(TWEN)
        // Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
        TWCR &= ~(_BV(TWEN));
    #endif

    // Make SDA (data) and SCL (clock) pins inputs with pullup
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);

    delay(2500);
    // Wait 2.5 secs. This is strictly only necessary on the first power
    // up of the DS3231 module to allow it to initialize properly,
    // but is also assists in reliable programming of FioV3 boards as it gives the
    // IDE a chance to start uploaded the program
    // before existing sketch confuses the IDE by sending Serial data.

    // If it is held low the device cannot become the I2C master
    // I2C bus error. Could not clear SCL clock line held low
    boolean scl_low = (digitalRead(scl) == LOW);
    if (scl_low) return 1;


    boolean sda_low = (digitalRead(sda) == LOW);
    int clockCount = 20; // > 2x9 clock

    // While SDA is low for at most 20 cycles
    while (sda_low && (clockCount > 0)) {

        clockCount--;

        // Note: I2C bus is open collector so do NOT drive SCL or SDA high
        pinMode(scl, INPUT);        // release SCL pullup so that when made output it will be LOW
        pinMode(scl, OUTPUT);       // then clock SCL Low
        delayMicroseconds(10);      // for >5uS
        pinMode(scl, INPUT);        // release SCL LOW
        pinMode(scl, INPUT_PULLUP); // turn on pullup resistors again
                                    // do not force high as slave may be holding it low for clock stretching

        delayMicroseconds(10);      // The >5uS is so that even the slowest I2C devices are handled

        //  loop waiting for SCL to become high only wait 2sec
        scl_low = (digitalRead(scl) == LOW);
        int counter = 20;
        while (scl_low && (counter > 0)) {
            counter--;
            delay(100);
            scl_low = (digitalRead(scl) == LOW);
        }

        // If still low after 2 sec error
        // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
        if (scl_low) return 2;

        sda_low = (digitalRead(sda) == LOW); //   and check SDA input again and loop

    }

    // If still low
    // I2C bus error. Could not clear. SDA data line held low
    if (sda_low) return 3;

    // Pull SDA line low for "start" or "repeated start"
    pinMode(sda, INPUT);        // remove pullup
    pinMode(sda, OUTPUT);       // and then make it LOW i.e. send an I2C Start or Repeated start control

    // When there is only one I2C master a "start" or "repeat start" has the same function as a "stop" and clears the bus
    // A Repeat Start is a Start occurring after a Start with no intervening Stop.

    delayMicroseconds(10);      // wait >5uS
    pinMode(sda, INPUT);        // remove output low
    pinMode(sda, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.

    delayMicroseconds(10);      // wait >5uS
    pinMode(sda, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
    pinMode(scl, INPUT);

    // Everything OK
    return 0;

}

bool i2cCheck(unsigned char address) {
    brzo_i2c_start_transaction(address, I2C_SCL_FREQUENCY);
    brzo_i2c_ACK_polling(1000);
    return brzo_i2c_end_transaction();
}

unsigned char i2cFind(size_t size, unsigned char * addresses) {
    for (unsigned char i=0; i<size; i++) {
        if (i2cCheck(addresses[i]) == 0) return addresses[i];
    }
    return 0;
}

void i2cScan() {
    unsigned char nDevices = 0;
    for (unsigned char address = 1; address < 127; address++) {
        unsigned char error = i2cCheck(address);
        if (error == 0) {
            DEBUG_MSG_P(PSTR("[I2C] Device found at address 0x%02X\n"), address);
            nDevices++;
        } else if (error != 32) {
            //DEBUG_MSG_P(PSTR("[I2C] Unknown error at address 0x%02X\n"), address);
        }
    }
    if (nDevices == 0) DEBUG_MSG_P(PSTR("[I2C] No devices found\n"));
}

void i2cSetup() {
    DEBUG_MSG_P(PSTR("[I2C] Using GPIO%d for SDA and GPIO%d for SCL\n"), I2C_SDA_PIN, I2C_SCL_PIN);
    //DEBUG_MSG_P(PSTR("[I2C] Clear bus (response: %d)\n"), _i2cClearbus(I2C_SDA_PIN, I2C_SCL_PIN));
    brzo_i2c_setup(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_STRETCH_TIME);
}

#endif
