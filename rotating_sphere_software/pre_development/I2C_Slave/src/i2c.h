// 

#ifndef I2C_H
#define I2C_H

#include <Arduino.h>
#include <i2c_slave.h>
#include <i2c_fifo.h>

class i2c {
  private:
  static const uint I2C_SLAVE_ADDRESS = 0x04;
  static const uint I2C_BAUDRATE = 400000; // 100 kHz
  
  static const uint I2C_SLAVE_SDA_PIN = 12;
  static const uint I2C_SLAVE_SCL_PIN = 13;

 
  static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);

  public:
    // The slave implements a 256 byte memory. To write a series of bytes, the master first
    // writes the memory address, followed by the data. The address is automatically incremented
    // for each byte transferred, looping back to 0 upon reaching the end. Reading is done
    // sequentially from the current memory address.
    static struct context_t
    {
      uint8_t mem[256];
      uint8_t mem_address;
      bool mem_address_written;
    } context;

    static int count;

    i2c();                                  // construktor
    static void setup_slave();
};

#endif
