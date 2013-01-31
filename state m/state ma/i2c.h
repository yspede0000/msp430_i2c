#ifndef I2C_H
#define I2C_H

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_rpt(void);
unsigned char i2c_write8(unsigned char c);
unsigned char i2c_read8(unsigned char acknack);

#endif

