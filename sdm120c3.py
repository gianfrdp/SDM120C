#!/usr/bin/env python
'''
Pymodbus Synchronous Client Examples
--------------------------------------------------------------------------

The following is an example of how to use the synchronous modbus client
implementation from pymodbus.

It should be noted that the client can also be used with
the guard construct that is available in python 2.5 and up::

    with ModbusClient('127.0.0.1') as client:
        result = client.read_coils(1,10)
        print result
'''
#---------------------------------------------------------------------------# 
# import the various server implementations
#---------------------------------------------------------------------------# 
#from pymodbus.client.sync import ModbusTcpClient as ModbusClient
#from pymodbus.client.sync import ModbusUdpClient as ModbusClient
from pymodbus.client.sync import ModbusSerialClient as ModbusClient

#---------------------------------------------------------------------------# 
# configure the client logging
#---------------------------------------------------------------------------# 
import logging
log = logging.getLogger()
log.setLevel(logging.DEBUG)
logpy = logging.getLogger("pymodbus")
logpy.setLevel(logging.INFO)
logging.basicConfig()



from struct import pack, unpack
from pymodbus.constants import Endian
from pymodbus.interfaces import IPayloadBuilder
from pymodbus.utilities import pack_bitstring
from pymodbus.utilities import unpack_bitstring
from pymodbus.exceptions import ParameterException
from pymodbus.payload import BinaryPayloadDecoder
from pymodbus.payload import BcdPayloadDecoder


#---------------------------------------------------------------------------# 
# choose the client you want
#---------------------------------------------------------------------------# 
# make sure to start an implementation to hit against. For this
# you can use an existing device, the reference implementation in the tools
# directory, or start a pymodbus server.
#
# If you use the UDP or TCP clients, you can override the framer being used
# to use a custom implementation (say RTU over TCP). By default they use the
# socket framer::
#
#    client = ModbusClient('localhost', port=5020, framer=ModbusRtuFramer)
#
# It should be noted that you can supply an ipv4 or an ipv6 host address for
# both the UDP and TCP clients.
#
# There are also other options that can be set on the client that controls
# how transactions are performed. The current ones are:
#
# * retries - Specify how many retries to allow per transaction (default = 3)
# * retry_on_empty - Is an empty response a retry (default = False)
# * source_address - Specifies the TCP source address to bind to
#
# Here is an example of using these options::
#
#    client = ModbusClient('localhost', retries=3, retry_on_empty=True)
#---------------------------------------------------------------------------# 
SLAVE = 1
#client = ModbusClient('localhost', port=502)
#client = ModbusClient(method='ascii', port='/dev/pts/2', timeout=1)
client = ModbusClient(method='rtu', port='/dev/ttyUSB0', stopbits=1, bytesize = 8, parity = 'N', baudrate = 9600, timeout=0.2, unit=SLAVE)

client.connect()

#---------------------------------------------------------------------------# 
# specify slave to query
#---------------------------------------------------------------------------# 
# The slave to query is specified in an optional parameter for each
# individual request. This can be done by specifying the `unit` parameter
# which defaults to `0x00`
#---------------------------------------------------------------------------# 
#rr = client.read_coils(1, 1, unit=0x02)

#---------------------------------------------------------------------------# 
# example requests
#---------------------------------------------------------------------------# 
# simply call the methods that you would like to use. An example session
# is displayed below along with some assert checks. Note that some modbus
# implementations differentiate holding/input discrete/coils and as such
# you will not be able to write to these, therefore the starting values
# are not known to these tests. Furthermore, some use the same memory
# blocks for the two sets, so a change to one is a change to the other.
# Keep both of these cases in mind when testing as the following will
# _only_ pass with the supplied async modbus server (script supplied).
#---------------------------------------------------------------------------# 
#rr = client.read_holding_registers(1,1)
#
#rr = client.read_holding_registers(address=0xF900, count=1)
#decoder = BcdPayloadDecoder.fromRegisters(rr.registers)
#Time_Interval = decoder.decode_int(2)
#Time_Interval = rs485.read_string(0xF900, functioncode=3, numberOfRegisters=2)[0]
#
rr = client.read_input_registers(address=0x0000, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Volts = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0006, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Current = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x000C, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Active_Power = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0012, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Apparent_Power = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0018, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Reactive_Power = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x001E, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Power_Factor = decoder.decode_32bit_float()
#
#Phase_Angle = master.execute(slave=SLAVE, function_code=cst.READ_INPUT_REGISTERS, starting_address=0x0006, quantity_of_x=2, data_format='>f')[0]
rr = client.read_input_registers(address=0x0046, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Frequency = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0048, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Import_Active_Energy = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x004A, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Export_Active_Energy  = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x004C, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Import_Reactive_Energy = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x004E, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Export_Reactive_Energy = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0156, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Total_Active_Energy = decoder.decode_32bit_float()
#
rr = client.read_input_registers(address=0x0158, count=2)
decoder = BinaryPayloadDecoder.fromRegisters(rr.registers, endian=Endian.Big)
Total_Reactive_Energy = decoder.decode_32bit_float()

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
#print('Time interval: %s' % str(Time_Interval))

#---------------------------------------------------------------------------# 
# close the client
#---------------------------------------------------------------------------# 
client.close()
