# SDM120C
SDM120C ModBus RTU client to read EASTRON SDM120C smart mini power meter registers

It depends on libmodbus (http://www.libmodbus.org)

To compile
  make clean && make

<PRE>
Usage: sdm120c [-a address] [-d] [-p] [-v] [-c] [-e] [-i] [-t] [-f] [-g] [-m] [-b baud_rate] device
       sdm120c [-a address] [-d] -s new_address device
       sdm120c [-a address] [-d] -r baud_rate device

where
        -a address      Meter number (between 1 and 247). Default: 1
        -s new_address  Set new meter number (between 1 and 247)
        -p              Get power (W)
        -v              Get voltage (V)
        -c              Get current (A)
        -f              Get frequency (Hz)
        -g              Get power factor
        -e              Get exported energy (kWh)
        -i              Get imported energy (kWh)
        -t              Get total energy (kWh)
        -d              Debug
        -b baud_rate    Use baud_rate serial port speed (1200, 2400, 4800, 9600)
                        Default: 2400
        -r baud_rate    Set baud_rate meter speed (1200, 2400, 4800, 9600)
        -m              Output values in IEC 62056 format ID(VALUE*UNIT)
        device          Serial device, i.e. /dev/ttyUSB0

Serial device is required. When no parameter is passed, retrives all values
</PRE>