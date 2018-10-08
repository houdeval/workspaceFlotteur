#!/usr/bin/python3
import smbus2
import time

bus = smbus2.SMBus(1)

I2C_ADD = 0x45
I2C_RESET = 0x1E
I2C_MEASURE = 0x48

I2C_STATE = 0xC1
I2C_VALUES = 0x00
I2C_PROM = 0xA2


prom = bus.read_i2c_block_data(I2C_PROM, 0, 10)
k4 = prom[0]<<8 | prom[1]
k3 = prom[2]<<8 | prom[3]
k2 = prom[4]<<8 | prom[5]
k1 = prom[6]<<8 | prom[7]
k0 = prom[8]<<8 | prom[9]

print("k0 = ", k0)
print("k1 = ", k1)
print("k2 = ", k2)
print("k3 = ", k3)
print("k4 = ", k4)

def compute_temperature(data):
    adc24 = data[0] << 16 | data[1] << 8 | data[0]
    adc16 = adc24/256.
    T = - 2.*k4*1e-21*(adc16**4) \
        + 4.*k3*1e-16*(adc16**3) \
        - 2.*k2*1e-11*(adc16**2) \
        + 1.*k1*1e-6*adc16 \
        -1.5*k0*1e-2
    return T

try:
    while True:
        # Ask measure
        bus.write_byte(I2C_MEASURE)

        time.sleep(0.04)

        # Read
        data = bus.read_i2c_block_data(I2C_VALUES, 0, 3)
        state = bus.read_byte(I2C_STATE)

        T = compute_temperature(data)

        print("T = ", T , "\tState = ", state, end =" ") 
        time.sleep(1./5.)

except KeyboardInterrupt:
    print('interrupted!')