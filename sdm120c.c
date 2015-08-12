#ifdef __cplusplus
extern "C" {
#endif

/*
 * sdm120c: ModBus RTU client to read EASTRON SDM120C smart mini power meter registers
 *
 * Copyright (C) 2015 Gianfranco Di Prinzio <gianfrdp@inwind.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>

#include <modbus-version.h>
#include <modbus.h>

#define DEFAULT_RATE 2400

#define MODEL_120 1
#define MODEL_220 2

// Read
#define VOLTAGE   0x0000
#define CURRENT   0x0006
#define POWER     0x000C
#define APOWER    0x0012
#define RAPOWER   0x0018
#define PFACTOR   0x001E
#define PANGLE    0x0024
#define FREQUENCY 0x0046
#define IAENERGY  0x0048
#define EAENERGY  0x004A
#define IRAENERGY 0x004C
#define ERAENERGY 0x004E
#define TAENERGY  0x0156
#define TRENERGY  0x0158

// Write
#define NPARSTOP  0x0012
#define DEVICE_ID 0x0014
#define BAUD_RATE 0x001C
#define TIME_DISP_220 0xF500
#define TIME_DISP 0xF900
#define TOT_MODE  0xF920

#define BR1200 5
#define BR2400 0
#define BR4800 1
#define BR9600 2

#define MAX_RETRIES 100

#define E_PARITY 'E'
#define O_PARITY 'O'
#define N_PARITY 'N'

#define RESTART_TRUE  1
#define RESTART_FALSE 0

int debug_flag     = 0;
const char *version = "1.1.5";

void usage(char* program) {
    printf("sdm120c %s: ModBus RTU client to read EASTRON SDM120C smart mini power meter registers\n",version);
    printf("Copyright (C) 2015 Gianfranco Di Prinzio <gianfrdp@inwind.it>\n");
    printf("Complied with libmodbus %s\n\n", LIBMODBUS_VERSION_STRING);
    printf("Usage: %s [-a address] [-d] [-p] [-v] [-c] [-e] [-i] [-t] [-f] [-g] [-T] [[-m]|[-q]] [-b baud_rate] [-P parity] [-S bit] [-z num_retries] [-j seconds] [-1 | -2] device\n", program);
    printf("       %s [-a address] [-d] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] -s new_address device\n", program);
    printf("       %s [-a address] [-d] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] -r baud_rate device \n", program);
    printf("       %s [-a address] [-d] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] -R new_time device\n\n", program);
    printf("where\n");
    printf("\t-a address \tMeter number (between 1 and 247). Default: 1\n");
    printf("\t-s new_address \tSet new meter number (between 1 and 247)\n");
    printf("\t-p \t\tGet power (W)\n");
    printf("\t-v \t\tGet voltage (V)\n");
    printf("\t-c \t\tGet current (A)\n");
    printf("\t-f \t\tGet frequency (Hz)\n");
    printf("\t-g \t\tGet power factor\n");
    printf("\t-e \t\tGet exported energy (Wh)\n");
    printf("\t-i \t\tGet imported energy (Wh)\n");
    printf("\t-t \t\tGet total energy (Wh)\n");
    printf("\t-T \t\tGet Time for rotating display values (0 = no rotation) \n");
    printf("\t-d \t\tDebug\n");
    printf("\t-b baud_rate \tUse baud_rate serial port speed (1200, 2400, 4800, 9600)\n");
    printf("\t\t\tDefault: 2400\n");
    printf("\t-P parity \tUse parity (E, N, O)\n");
    printf("\t-S bit \t\tUse stop bits (1, 2). Default: 1\n");
    printf("\t-r baud_rate \tSet baud_rate meter speed (1200, 2400, 4800, 9600)\n");
    printf("\t-R new_time \tChange rotation time for displaying values (0 - 30s) (0 = no totation)\n");
    printf("\t-m \t\tOutput values in IEC 62056 format ID(VALUE*UNIT)\n");
    printf("\t-q \t\tOutput values in compact mode\n");
    printf("\t-z num_retries\tTry to read max num_retries times on bus before exiting\n");
    printf("\t\t\twith error. Default: 1\n");
    printf("\t-j seconds\tResponse timeout. Default: 2\n");
    printf("\t-1 \t\tModel: SDM120C (defualt)\n");
    printf("\t-2 \t\tModel: SDM220\n");
    printf("\tdevice\t\tSerial device, i.e. /dev/ttyUSB0\n\n");
    printf("Serial device is required. When no parameter is passed, retrives all values\n");
}

inline int bcd2int(int val)
{
    return((((val & 0xf0) >> 4) * 10) + (val & 0xf));
}

int int2bcd(int val)
{
    return(((val / 10) << 4) + (val % 10));
}

int bcd2num(const uint16_t *src, int len)
{
    int n = 0;
    int m = 1;
    int i = 0;
    int shift = 0;
    int digit = 0;
    int j = 0;
    for (i = 0; i < len; i++) {
        for (j = 0; j < 4; j++) {
            digit = ((src[len-1-i]>>shift) & 0x0F) * m;
            n += digit;
            m *= 10;
            shift += 4;
        }
    }
    return n;
}

int getMeasureBCD(modbus_t *ctx, int address, int retries, int nb) {

    uint16_t tab_reg[nb * sizeof(uint16_t)];
    int rc;
    int i;
    int j = 0;
    int exit_loop = 0;

    while (j < retries && exit_loop == 0) {
      j++;

      if (debug_flag == 1) {
         printf("%d/%d. Register Address %d [%04X]\n", j, retries, 30000+address+1, address);
      }
      rc = modbus_read_input_registers(ctx, address, nb, tab_reg);

      if (rc == -1) {
        fprintf(stderr, "ERROR %s, %d\n", modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }

    if (rc == -1) {
      modbus_close(ctx);
      modbus_free(ctx);
      exit(EXIT_FAILURE);
    }

    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          printf("reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
       }
    }

    int value = bcd2num(&tab_reg[0], rc);

    return value;
}

float getMeasureFloat(modbus_t *ctx, int address, int retries, int nb) {

    uint16_t tab_reg[nb * sizeof(uint16_t)];
    int rc;
    int i;
    int j = 0;
    int exit_loop = 0;
    
    while (j < retries && exit_loop == 0) {
      j++;
      
      if (debug_flag == 1) {
         printf("%d/%d. Register Address %d [%04X]\n", j, retries, 30000+address+1, address); 
      }
      rc = modbus_read_input_registers(ctx, address, nb, tab_reg);
      
      if (rc == -1) {
        fprintf(stderr, "ERROR %s, %d\n", modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }
    
    if (rc == -1) {
      modbus_close(ctx);
      modbus_free(ctx);
      exit(EXIT_FAILURE);
    }
    
    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          printf("reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
       }
    }

    // swap LSB and MSB
    uint16_t tmp1 = tab_reg[0];
    uint16_t tmp2 = tab_reg[1];
    tab_reg[0] = tmp2;
    tab_reg[1] = tmp1;

    float value = modbus_get_float(&tab_reg[0]);

    return value;

}

int getConfigBCD(modbus_t *ctx, int address, int retries, int nb) {

    uint16_t tab_reg[nb * sizeof(uint16_t)];
    int rc;
    int i;
    int j = 0;
    int exit_loop = 0;

    while (j < retries && exit_loop == 0) {
      j++;

      if (debug_flag == 1) {
         printf("%d/%d. Register Address %d [%04X]\n", j, retries, 400000+address+1, address);
      }
      rc = modbus_read_registers(ctx, address, nb, tab_reg);

      if (rc == -1) {
        fprintf(stderr, "ERROR %s, %d\n", modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }

    if (rc == -1) {
      modbus_close(ctx);
      modbus_free(ctx);
      exit(EXIT_FAILURE);
    }

    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          printf("reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
       }
    }

    int value = bcd2num(&tab_reg[0], rc);

    return value;

}

void changeConfigFloat(modbus_t *ctx, int address, int new_value, int restart, int nb)
{
    uint16_t tab_reg[nb * sizeof(uint16_t)];

    modbus_set_float((float) new_value, &tab_reg[0]);
    // swap LSB and MSB
    uint16_t tmp1 = tab_reg[0];
    uint16_t tmp2 = tab_reg[1];
    tab_reg[0] = tmp2;
    tab_reg[1] = tmp1;

    int n = modbus_write_registers(ctx, address, nb, tab_reg);
    if (n != -1) {
        printf("New value %d for address 0x%X\n", new_value, address);
        if (restart == RESTART_TRUE) printf("You have to restart the meter for apply changes\n");
    } else {
        printf("errno: %s, %d, %d\n", modbus_strerror(errno), errno, n);
        modbus_close(ctx);
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }
}

void changeConfigBCD(modbus_t *ctx, int address, int new_value, int restart, int nb)
{
    uint16_t tab_reg[nb * sizeof(uint16_t)];
    uint16_t u_new_value = int2bcd(new_value);
    tab_reg[0] = u_new_value;

    int n = modbus_write_registers(ctx, address, nb, tab_reg);
    if (n != -1) {
        printf("New value %d for address 0x%X\n", u_new_value, address);
        if (restart == RESTART_TRUE) printf("You have to restart the meter for apply changes\n");
    } else {
        printf("errno: %s, %d, %d\n", modbus_strerror(errno), errno, n);
        modbus_close(ctx);
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{

    int device_address = 1;
    int model          = MODEL_120;
    int new_address    = 0;
    int power_flag     = 0;
    int volt_flag      = 0;
    int current_flag   = 0;
    int freq_flag      = 0;
    int pf_flag        = 0;
    int apower_flag    = 0;
    int rapower_flag   = 0;
    int export_flag    = 0;
    int import_flag    = 0;
    int total_flag     = 0;
    int baud_rate      = 0;
    int stop_bits      = 1;
    int new_baud_rate  = 0;
    int metern_flag    = 0;
    int compact_flag   = 0;
    int time_disp_flag = 0;
    int rotation_time_flag = 0;
    int rotation_time  = 0; 
    int count_param    = 0;
    int num_retries    = 1;
#if LIBMODBUS_VERSION_MAJOR >= 3 && LIBMODBUS_VERSION_MINOR >= 1 && LIBMODBUS_VERSION_MICRO >= 2
    uint32_t resp_timeout = 2;
#else
    time_t resp_timeout   = 2;
#endif
    int index;
    int c;
    char *device       = NULL;
    char *c_parity     = NULL;
    int speed          = 0;
    int bits           = 0;
    int read_count     = 0;
    int stop_bits      = 2;

    const char *EVEN_parity = "E";
    const char *NONE_parity = "N";
    const char *ODD_parity  = "O";
    char parity             = N_PARITY;

    if (argc == 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    opterr = 0;

    while ((c = getopt (argc, argv, "a:b:cdefgij:lmnpP:qr:R:s:S:tTvz:12")) != -1) {
        switch (c)
        {
            case 'a':
                device_address = atoi(optarg);
                if (!(0 < device_address && device_address <= 247)) {
                    fprintf (stderr, "Address must be between 1 and 247.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'v':
                volt_flag = 1;
                count_param++;
                break;
            case 'p':
                power_flag = 1;
                count_param++;
                break;
            case 'c':
                current_flag = 1;
                count_param++;
                break;
            case 'e':
                export_flag = 1;
                count_param++;
                break;
            case 'i':
                import_flag = 1;
                count_param++;
                break;
            case 't':
                total_flag = 1;
                count_param++;
                break;
            case 'f':
                freq_flag = 1;
                count_param++;
                break;
            case 'g':
                pf_flag = 1;
                count_param++;
                break;
            case 'l':
                apower_flag = 1;
                count_param++;
                break;
            case 'n':
                rapower_flag = 1;
                count_param++;
                break;
            case 'd':
                debug_flag = 1;
                break;
            case 'b':
                speed = atoi(optarg);
                if (speed == 1200 || speed == 2400 || speed == 4800 || speed == 9600) {
                    baud_rate = speed;
                } else {
                    fprintf (stderr, "Baud Rate must be one of 1200, 2400, 4800, 9600\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'P':
                //  The Modbus specification (page 12) states:
                //  "Devices may accept by configuration either Even, Odd, or No Parity checking.
                //  If No Parity is implemented, an additional stop bit is transmitted to fill out
                //  the character frame to a full 11-bit asynchronous character"
                c_parity = strdup(optarg);
                if (strcmp(c_parity,EVEN_parity) == 0) {
                    parity = E_PARITY;
                    stop_bits = 1;
                } else if (strcmp(c_parity,NONE_parity) == 0) {
                    parity = N_PARITY;
                    stop_bits = 2;
                } else if (strcmp(c_parity,ODD_parity) == 0) {
                    parity = O_PARITY;
                    stop_bits = 1;
                } else {
                    fprintf (stderr, "Parity must be one of E, N, O\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'S':
                bits = atoi(optarg);
                if (bits == 1 || bits == 2) {
                    stop_bits = bits;
                } else {
                    fprintf (stderr, "Stop bits can be one of 1, 2\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r':
                speed = atoi(optarg);
                switch (speed) {
                    case 1200:
                        new_baud_rate = BR1200;
                        break;
                    case 2400:
                        new_baud_rate = BR2400;
                        break;
                    case 4800:
                        new_baud_rate = BR4800;
                        break;
                    case 9600:
                        new_baud_rate = BR9600;
                        break;
                    default:
                        fprintf (stderr, "Baud Rate must be one of 1200, 2400, 4800, 9600\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 's':
                new_address = atoi(optarg);
                if (!(0 < new_address && new_address <= 247)) {
                    fprintf (stderr, "New address must be between 1 and 247.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'R':
                rotation_time_flag = 1;
                rotation_time = atoi(optarg);

                if (!(0 <= rotation_time && rotation_time <= 30)) {
                    fprintf (stderr, "New rotation time must be between 0 and 30.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case '1':
                model = MODEL_120;
                break;
            case '2':
                model = MODEL_220;
                break;
            case 'm':
                metern_flag = 1;
                break;
            case 'q':
                compact_flag = 1;
                break;
            case 'z':
                num_retries = atoi(optarg);
                if (!(0 < num_retries && num_retries <= MAX_RETRIES)) {
                    fprintf (stderr, "num_reties must be between 1 and %d.\n", MAX_RETRIES);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'j':
                resp_timeout = atoi(optarg);
                break;
            case 'T':
                time_disp_flag = 1;
                count_param++;
                break;
            case '?':
                if (optopt == 'a' || optopt == 'b' || optopt == 's') {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                else if (isprint (optopt)) {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                else {
                    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
            default:
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if ((optind+1) > argc) {
        /* need at least one argument (change +1 to +2 for two, etc. as needeed) */
        if (debug_flag == 1) {
           printf("optind = %d, argc = %d\n", optind, argc);
        }
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else {
        device = argv[optind];
    }

    if (compact_flag == 1 && metern_flag == 1) {
        fprintf(stderr, "Parameter -m and -q are mutually exclusive\n\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    modbus_t *ctx;
    if (baud_rate == 0) baud_rate = DEFAULT_RATE;

    ctx = modbus_new_rtu(device, baud_rate, parity, 8, stop_bits);

    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        exit(EXIT_FAILURE);
    }

#if LIBMODBUS_VERSION_MAJOR >= 3 && LIBMODBUS_VERSION_MINOR >= 1 && LIBMODBUS_VERSION_MICRO >= 2

    uint32_t old_response_to_sec;
    uint32_t old_response_to_usec;

    modbus_get_response_timeout(ctx, &old_response_to_sec, &old_response_to_usec);
    
    // Considering to get those values from command line
    modbus_set_byte_timeout(ctx, -1, 0);
    modbus_set_response_timeout(ctx, resp_timeout, 0);

#else

    struct timeval old_response_timeout;
    struct timeval response_timeout;
    
    modbus_get_response_timeout(ctx, &old_response_timeout);
    response_timeout.tv_sec = -1;
    response_timeout.tv_usec = 0;
    modbus_set_byte_timeout(ctx, &response_timeout);
    response_timeout.tv_sec = resp_timeout;
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(ctx, &response_timeout);

#endif

    modbus_set_error_recovery(ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
                      
    if (debug_flag == 1) {
        modbus_set_debug(ctx, 1);
    }
    int slave = 0;
    slave = modbus_set_slave(ctx, device_address);
    
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        exit(EXIT_FAILURE);
    }

    float voltage    = 0;
    float current    = 0;
    float power      = 0;
    float apower     = 0;
    float rapower    = 0;
    float pf         = 0;
    float freq       = 0;
    float imp_energy = 0;
    float exp_energy = 0;
    float tot_energy = 0;
    int   time_disp  = 0;

    if (new_address > 0 && new_baud_rate > 0) {
        
        fprintf(stderr, "Parameter -s and -r are mutually exclusive\n\n");
        usage(argv[0]);
        modbus_close(ctx);
        modbus_free(ctx);
        exit(EXIT_FAILURE);
        
    } else if (new_address > 0) {
    
        if (count_param > 0) {
            usage(argv[0]);
            modbus_close(ctx);
            modbus_free(ctx);
            exit(EXIT_FAILURE);
        } else {
            // change Address
            changeConfigFloat(ctx, DEVICE_ID, new_address, RESTART_TRUE, 2);
            modbus_close(ctx);
            modbus_free(ctx);
            return 0;
        }
        
    } else if (new_baud_rate > 0) {
    
        if (count_param > 0) {
            usage(argv[0]);
            modbus_close(ctx);
            modbus_free(ctx);
            exit(EXIT_FAILURE);
        } else {
            // change Baud Rate
            changeConfigFloat(ctx, BAUD_RATE, new_baud_rate, RESTART_TRUE, 2);
            modbus_close(ctx);
            modbus_free(ctx);
            return 0;
        }
        
    } else if (rotation_time_flag > 0) {

        if (count_param > 0) {
            usage(argv[0]);
            modbus_close(ctx);
            modbus_free(ctx);
            exit(EXIT_FAILURE);
        } else {
            // change Time Rotation
            if (model == MODEL_120) {
                changeConfigBCD(ctx, TIME_DISP, rotation_time, RESTART_FALSE, 1);
            } else {
                changeConfigBCD(ctx, TIME_DISP_220, rotation_time, RESTART_FALSE, 1);
            }
            modbus_close(ctx);
            modbus_free(ctx);
            return 0;
        }

    } else if (power_flag   == 0 &&
               volt_flag    == 0 &&
               current_flag == 0 &&
               freq_flag    == 0 &&
               pf_flag      == 0 &&
               export_flag  == 0 &&
               import_flag  == 0 &&
               total_flag   == 0 &&
               time_disp_flag == 0
       ) {
       // if no parameter, retrieve all values
        power_flag   = 1;
        volt_flag    = 1;
        current_flag = 1;
        freq_flag    = 1;
        pf_flag      = 1;
        export_flag  = 1;
        import_flag  = 1;
        total_flag   = 1;
        count_param  = power_flag + volt_flag + current_flag + freq_flag + pf_flag + export_flag + import_flag + total_flag;
    }

    if (volt_flag == 1) {
        voltage = getMeasureFloat(ctx, VOLTAGE, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%4.2f*V)\n", device_address, voltage);
        } else if (compact_flag == 1) {
            printf("%4.2f ", voltage);
        } else {
            printf("Voltage: %4.2f V \n",voltage);
        }
    }

    if (current_flag == 1) {
        current  = getMeasureFloat(ctx, CURRENT, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%4.2f*A)\n", device_address, current);
        } else if (compact_flag == 1) {
            printf("%4.2f ", current);
        } else {
            printf("Current: %4.2f A \n",current);
        }
    }

     if (power_flag == 1) {
        power = getMeasureFloat(ctx, POWER, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%5.2f*W)\n", device_address, power);
        } else if (compact_flag == 1) {
            printf("%4.2f ", power);
        } else {
            printf("Power: %5.2f W \n", power);
        }
    }

    if (apower_flag == 1) {
        apower = getMeasureFloat(ctx, APOWER, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%5.2f*VA)\n", device_address, apower);
        } else if (compact_flag == 1) {
            printf("%5.2f ", apower);
        } else {
            printf("Active Apparent Power: %5.2f VA \n", apower);
        }
    }

    if (rapower_flag == 1) {
        rapower = getMeasureFloat(ctx, RAPOWER, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%5.2f*VAr)\n", device_address, rapower);
        } else if (compact_flag == 1) {
            printf("%4.2f ", rapower);
        } else {
            printf("Reactive Apparent Power: %5.2f VAr \n", rapower);
        }
    }

    if (pf_flag == 1) {
        pf = getMeasureFloat(ctx, PFACTOR, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%4.2f*-)\n", device_address, pf);
        } else if (compact_flag == 1) {
            printf("%4.2f ", pf);
        } else {
            printf("Power Factor: %4.2f \n", pf);
        }
    }

    if (freq_flag == 1) {
        freq = getMeasureFloat(ctx, FREQUENCY, num_retries, 2);
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%4.2f*Hz)\n", device_address, freq);
        } else if (compact_flag == 1) {
            printf("%4.2f ", freq);
        } else {
            printf("Frequency: %4.2f Hz \n", freq);
        }
    }

    if (import_flag == 1) {
        imp_energy = getMeasureFloat(ctx, IAENERGY, num_retries, 2) * 1000;
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%d*Wh)\n", device_address, (int)(imp_energy));
        } else if (compact_flag == 1) {
            printf("%d ", (int) imp_energy);
        } else {
            printf("Import Active Energy: %d Wh \n", (int)(imp_energy));
        }
    }

    if (export_flag == 1) {
        exp_energy = getMeasureFloat(ctx, EAENERGY, num_retries, 2) * 1000;
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%d*Wh)\n", device_address, (int)(exp_energy));
        } else if (compact_flag == 1) {
            printf("%d ", (int) exp_energy);
        } else {
            printf("Export Active Energy: %d Wh \n", (int)(exp_energy));
        }
    }

    if (total_flag == 1) {
        tot_energy = getMeasureFloat(ctx, TAENERGY, num_retries, 2) * 1000;
        read_count++;
        if (metern_flag == 1) {
            printf("%d(%d*Wh)\n", device_address, (int)(tot_energy));
        } else if (compact_flag == 1) {
            printf("%d ", (int) tot_energy);
        } else {
            printf("Total Active Energy: %d Wh \n", (int)(tot_energy));
        }
    }

    if (time_disp_flag == 1) {
        time_disp = getConfigBCD(ctx, TIME_DISP, num_retries, 1);
        read_count++;
        if (compact_flag == 1) {
            printf("%d ", (int) time_disp);
        } else {
            printf("Display rotation time: %d\n", time_disp);
        }
    }

    if (read_count == count_param) {
        printf(" OK\n");
    } else {
        printf(" NOK\n");
    }
    
    /*
    if (compact_flag == 1) {
        printf("\n");
    }
    */

    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}

#ifdef __cplusplus
}
#endif
