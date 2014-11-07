#ifndef I2C_MASTER_INTERFACE_H
#define I2C_MASTER_INTERFACE_H


void i2c_master_interface_init(void);

bool i2c_master_read_reg(uint8_t slave_addr,
                         uint8_t * tx_data, uint8_t tx_len,
                         uint8_t * rx_data, uint8_t rx_len);

bool i2c_master_write_reg(uint8_t slave_addr, uint8_t * data, uint8_t len);

extern struct i2c_master_module i2c_master_instance;

#endif // I2C_MASTER_INTERFACE_H
