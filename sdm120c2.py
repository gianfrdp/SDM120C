# -*- coding: utf_8 -*-
"""
 Modbus TestKit: Implementation of Modbus protocol in python
 (C)2009 - Luc Jean - luc.jean@gmail.com
 (C)2009 - Apidev - http://www.apidev.fr
 This is distributed under GNU LGPL license, see license.txt
"""

from struct import *
import serial
import modbus_tk
import modbus_tk.defines as cst
from modbus_tk import modbus_rtu

def read_float(arrRegs):
    reg1 = arrRegs[1]
    reg2 = arrRegs[0]
    f = unpack('f', pack('<HH', reg1, reg2))[0]
    print(arrRegs)
    print('reg0: %s' % str(reg1))
    print('reg1: %s' % str(reg2))
    print('f: %.1f' % f)
    back1, back2 = unpack('<HH',pack('f',f))
    print('back1: %s, back2: %s' % (str(back1),str(back2)))
    return f

def bcdDigits(chars):
    for char in chars:
        #char = ord(char)
        for val in (char >> 4, char & 0xF):
            if val == 0xF:
                return
            yield val

PORT = '/dev/ttyUSB0'
SLAVE = 1

def main():
    """main"""
    logger = modbus_tk.utils.create_logger("console")

    try:
        #Connect to the slave
        master = modbus_rtu.RtuMaster(
            serial.Serial(port=PORT, baudrate=9600, bytesize=8, parity='N', stopbits=1, xonxoff=0)
        )
        master.set_timeout(5.0)
        master.set_verbose(True)
        logger.info("connected")

        Volts = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0000, quantity_of_x=2, data_format='>f')[0]
        Current = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0006, quantity_of_x=2, data_format='>f')[0]
        Active_Power = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x000C, quantity_of_x=2, data_format='>f')[0]
        Apparent_Power = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0012, quantity_of_x=2, data_format='>f')[0]
        Reactive_Power = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0018, quantity_of_x=2, data_format='>f')[0]
        Power_Factor = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x001E, quantity_of_x=2, data_format='>f')[0]
        #Phase_Angle = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0006, quantity_of_x=2, data_format='>f')[0]
        Frequency = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0046, quantity_of_x=2, data_format='>f')[0]
        Import_Active_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0048, quantity_of_x=2, data_format='>f')[0]
        Export_Active_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x004A, quantity_of_x=2, data_format='>f')[0]
        Import_Reactive_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x004C, quantity_of_x=2, data_format='>f')[0]
        Export_Reactive_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x004E, quantity_of_x=2, data_format='>f')[0]
        Total_Active_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0156, quantity_of_x=2, data_format='>f')[0]
        Total_Reactive_Energy = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0158, quantity_of_x=2, data_format='>f')[0]
        #Time_Interval = rs485.read_string(0xF900, functioncode=3, numberOfRegisters=2)[0]
        Time_Interval = master.execute(slave=SLAVE, function_code=cst.READ_HOLDING_REGISTERS, starting_address=0xF900, quantity_of_x=1, data_format='>H')[0]

        #send some queries
        #logger.info('Voltage: %.1f Volts' % read_float(master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0000, quantity_of_x=2)))
        print('Voltage: %.1f Volts' % (Volts))
        print('Current: %.1f Amps' % (Current))
        print('Active power: %.1f Watts' % (Active_Power))
        print('Apparent power: %.1f VoltAmps' % (Apparent_Power))
        print('Reactive power: %.1f VAr' % (Reactive_Power))
        print('Power factor: %.1f' % (Power_Factor))
        #print('Phase angle: %.1f Degree' % (Phase_Angle))
        print('Frequency: %.1f Hz' % (Frequency))
        print('Import active energy: %.3f Kwh' % (Import_Active_Energy))
        print('Export active energy: %.3f kwh' % (Export_Active_Energy))
        print('Import reactive energy: %.3f kvarh' % (Import_Reactive_Energy))
        print('Export reactive energy: %.3f kvarh' % (Export_Reactive_Energy))
        print('Total active energy: %.3f kwh' % (Total_Active_Energy))
        print('Total reactive energy: %.3f kvarh' % (Total_Reactive_Energy))
        #print('Current Yield (V*A): %.1f Watt' % (Volts * Current))
        print('Current Yield (V*A): %.1f Watt' % (Volts*Current))
        print('Time interval: %d' % (Time_Interval))
        #print(Time_Interval)
        arrRegs = master.execute(slave=SLAVE, function_code=cst.READ_HOLDING_REGISTERS, starting_address=0xF900, quantity_of_x=1, data_format='>BB')
        print(arrRegs)
        print(list(bcdDigits(arrRegs)))

    except modbus_tk.modbus.ModbusError as exc:
        logger.error("%s- Code=%d", exc, exc.get_exception_code())

if __name__ == "__main__":
    main()

