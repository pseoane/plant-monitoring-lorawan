#include "Si7021.h"
 
Si7021::Si7021(PinName sda, PinName scl):i2c(sda, scl)
{
    i2c.frequency(FREQ);
}
 
Si7021::~Si7021()
{
    
}
 
float Si7021::get_temperature()
{
	float temp = float(tData / 1000 + (tData % 1000)*0.001);
	return temp;
}
 
float Si7021::get_humidity()
{
	float hum = float(rhData / 1000 + (rhData % 1000)*0.001);
	return hum;
}
 
bool Si7021::measure()
{
    tx_buff[0] = READ_RH;
    if(i2c.write(ADDR, (char*)tx_buff, 1) != 0) return 0;
    if(i2c.read(ADDR, (char*)rx_buff, 2) != 0) return 0;
    
    rhData = ((uint32_t)rx_buff[0] << 8) + (rx_buff[1] & 0xFC);
    rhData = (((rhData) * 15625L) >> 13) - 6000;
    
    tx_buff[0] = READ_TEMP;
    if(i2c.write(ADDR, (char*)tx_buff, 1) != 0) return 0;
    if(i2c.read(ADDR, (char*)rx_buff, 2) != 0) return 0;
    
    tData = ((uint32_t)rx_buff[0] << 8) + (rx_buff[1] & 0xFC);
    tData = (((tData) * 21965L) >> 13) - 46850;
    
    return 1;
}
 
bool Si7021::check()
{
    tx_buff[0] = READ_ID2_1;
    tx_buff[1] = READ_ID2_2;
    if(i2c.write(ADDR, (char*)tx_buff, 2) != 0) return 0;
    if(i2c.read(ADDR, (char*)rx_buff, 8) != 0) return 0;
    
    if(rx_buff[0] == DEVICE_ID)
        return true;
    else return 0;
}