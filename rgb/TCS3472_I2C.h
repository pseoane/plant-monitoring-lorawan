#ifndef TCS3472_I2C_H
#define TCS3472_I2C_H
#include "mbed.h"
 
//Defines 
#define SLAVE_ADDRESS           0x29
#define ENABLE                  0x00
#define CDATA                   0x14

/** TCS3472_I2C class.
 *  Used to read to and write from TCS3472 color sensor.
 */
class TCS3472_I2C {
public:
    /** Create TCS3472_I2C instance
     *
     * @param sda sda pin for I2C
     * @param scl scl pin for I2C
     */
    TCS3472_I2C( PinName sda, PinName scl );
    
    /** Destructor
     */
    ~TCS3472_I2C();
    
    /** Read red, green, blue and clear values into array
     *
     * @param readings Array of four integers to store the read data
		 * @returns
		 *   Predominant color (0 = RED, 1 = GREEN, 2 = BLUE)
     */
    int getAllColors(uint16_t* readings );
    
    /** Activates internal oscillator and two-channel ADC simultaneously (both necessary for standard operation).
     *
     * @returns
     *     1 if successful
     *     0 if otherwise
     */
    int enablePowerAndRGBC();
    
    /** Disables internal oscillator and two-channel ADC simultaneously.
     *
     * @returns
     *     1 if successful
     *     0 if otherwise
     */
    int disablePowerAndRGBC();

 
private:
    I2C i2c;
    
    int writeSingleRegister( char address, char data );
    int writeMultipleRegisters( char address, char* data, int quantity );
    char readSingleRegister( char address );
    int readMultipleRegisters( char address, char* output, int quantity );
};
#endif