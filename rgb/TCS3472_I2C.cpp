#include "TCS3472_I2C.h"
 
TCS3472_I2C::TCS3472_I2C( PinName sda, PinName scl ) : i2c( sda, scl ){   
    i2c.frequency(100000);
}
 
TCS3472_I2C::~TCS3472_I2C(){
    
}
 
int TCS3472_I2C::writeSingleRegister( char address, char data ){
    char tx[2];
		tx[0] =  address | 160; //0d160 = 0b10100000
		tx[1]= data;
    int ack = i2c.write( SLAVE_ADDRESS << 1, tx, 2 );
    return ack;
}
 
int TCS3472_I2C::writeMultipleRegisters( char address, char* data, int quantity ){
    char tx[ quantity + 1 ];
    tx[0] = address | 160;
    for ( int i = 1; i <= quantity; i++ ){
        tx[ i ] = data[ i - 1 ];
    }
    int ack = i2c.write( SLAVE_ADDRESS << 1, tx, quantity + 1 );
    return ack;
}
 
char TCS3472_I2C::readSingleRegister( char address ){
    char output = 255;
    char command = address | 160; //0d160 = 0b10100000
    i2c.write( SLAVE_ADDRESS << 1, &command, 1, true );
    i2c.read( SLAVE_ADDRESS << 1, &output, 1 );
    return output;
}
 
int TCS3472_I2C::readMultipleRegisters( char address, char* output, int quantity ){
    char command = address | 160; //0d160 = 0b10100000
    i2c.write( SLAVE_ADDRESS << 1, &command, 1, true );
    int ack = i2c.read( SLAVE_ADDRESS << 1, output, quantity );
    return ack;
}
 
int TCS3472_I2C::getAllColors(uint16_t* readings){
    char buffer[8] = { 0 };
 
    readMultipleRegisters( CDATA, buffer, 8 );
 
    readings[0] = (int)buffer[1] << 8 | (int)buffer[0];
    readings[1] = (int)buffer[3] << 8 | (int)buffer[2];
    readings[2] = (int)buffer[5] << 8 | (int)buffer[4];
    readings[3] = (int)buffer[7] << 8 | (int)buffer[6];
		
	  return 0;
}
 
int TCS3472_I2C::enablePowerAndRGBC(){
    char enable_old = readSingleRegister( ENABLE );
    char enable_new = enable_old | 3; // sets PON (bit 0) and AEN (bit 1) to 1
    int ack = writeSingleRegister( ENABLE, enable_new );
    return ack;
}
 
int TCS3472_I2C::disablePowerAndRGBC(){
    char enable_old = readSingleRegister( ENABLE );
    char enable_new = enable_old & 252; // sets PON (bit 0) and AEN (bit 1) to 0
    int ack = writeSingleRegister( ENABLE, enable_new );
    return ack;
}