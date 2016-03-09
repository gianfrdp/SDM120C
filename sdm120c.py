#!/usr/bin/python

import minimalmodbus

rs485 = minimalmodbus.Instrument('/dev/ttyUSB0', 1)
rs485.serial.baudrate = 9600
rs485.serial.bytesize = 8
rs485.serial.parity = minimalmodbus.serial.PARITY_NONE
rs485.serial.stopbits = 1
rs485.serial.timeout = 1
rs485.debug = False
rs485.mode = minimalmodbus.MODE_RTU
print rs485

Volts = rs485.read_float(0, functioncode=4, numberOfRegisters=2)
Current = rs485.read_float(6, functioncode=4, numberOfRegisters=2)
Active_Power = rs485.read_float(12, functioncode=4, numberOfRegisters=2)
Apparent_Power = rs485.read_float(18, functioncode=4, numberOfRegisters=2)
Reactive_Power = rs485.read_float(24, functioncode=4, numberOfRegisters=2)
Power_Factor = rs485.read_float(30, functioncode=4, numberOfRegisters=2)
Phase_Angle = rs485.read_float(36, functioncode=4, numberOfRegisters=2)
Frequency = rs485.read_float(70, functioncode=4, numberOfRegisters=2)
Import_Active_Energy = rs485.read_float(72, functioncode=4, numberOfRegisters=2) 
Export_Active_Energy = rs485.read_float(74, functioncode=4, numberOfRegisters=2)
Import_Reactive_Energy = rs485.read_float(76, functioncode=4, numberOfRegisters=2)
Export_Reactive_Energy = rs485.read_float(78, functioncode=4, numberOfRegisters=2)
Total_Active_Energy = rs485.read_float(342, functioncode=4, numberOfRegisters=2)
Total_Reactive_Energy = rs485.read_float(344, functioncode=4, numberOfRegisters=2)

print 'Voltage: {0:.1f} Volts'.format(Volts)
print 'Current: {0:.1f} Amps'.format(Current)
print 'Active power: {0:.1f} Watts'.format(Active_Power)
print 'Apparent power: {0:.1f} VoltAmps'.format(Apparent_Power)
print 'Reactive power: {0:.1f} VAr'.format(Reactive_Power)
print 'Power factor: {0:.1f}'.format(Power_Factor)
print 'Phase angle: {0:.1f} Degree'.format(Phase_Angle)
print 'Frequency: {0:.1f} Hz'.format(Frequency)
print 'Import active energy: {0:.3f} Kwh'.format(Import_Active_Energy)
print 'Export active energy: {0:.3f} kwh'.format(Export_Active_Energy)
print 'Import reactive energy: {0:.3f} kvarh'.format(Import_Reactive_Energy)
print 'Export reactive energy: {0:.3f} kvarh'.format(Export_Reactive_Energy)
print 'Total active energy: {0:.3f} kwh'.format(Total_Active_Energy)
print 'Total reactive energy: {0:.3f} kvarh'.format(Total_Reactive_Energy)
print 'Current Yield (V*A): {0:.1f} Watt'.format(Volts * Current)
