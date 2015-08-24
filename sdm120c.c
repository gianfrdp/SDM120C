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

#include <sys/types.h>
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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
int EXIT_ERROR     = 0;
int trace_flag     = 0;

const char *version     = "1.2.0";
char *programName = "sdm120c";
const char *ttyLCKloc   = "/var/lock/LCK.."; /* location and prefix of serial port lock file */

long unsigned int PID;
static int yLockWait = 0; /* Seconds to wait to lock serial port */
char *devLCKfile = NULL;
char *devLCKfileNew = NULL;

void usage(char* program) {
    printf("sdm120c %s: ModBus RTU client to read EASTRON SDM120C smart mini power meter registers\n",version);
    printf("Copyright (C) 2015 Gianfranco Di Prinzio <gianfrdp@inwind.it>\n");
    printf("Complied with libmodbus %s\n\n", LIBMODBUS_VERSION_STRING);
    printf("Usage: %s [-a address] [-d] [-x] [-p] [-v] [-c] [-e] [-i] [-t] [-f] [-g] [-T] [[-m]|[-q]] [-b baud_rate] [-P parity] [-S bit] [-z num_retries] [-j seconds] [-w seconds] [-1 | -2] device\n", program);
    printf("       %s [-a address] [-d] [-x] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] [-z num_retries] [-j seconds] [-w seconds] -s new_address device\n", program);
    printf("       %s [-a address] [-d] [-x] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] [-z num_retries] [-j seconds] [-w seconds] -r baud_rate device \n", program);
    printf("       %s [-a address] [-d] [-x] [-b baud_rate] [-P parity] [-S bit] [-1 | -2] [-z num_retries] [-j seconds] [-w seconds] -R new_time device\n\n", program);
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
    printf("\t-x \t\tTrace (libmodbus debug on)\n");
    printf("\t-b baud_rate \tUse baud_rate serial port speed (1200, 2400, 4800, 9600)\n");
    printf("\t\t\tDefault: 2400\n");
    printf("\t-P parity \tUse parity (E, N, O)\n");
    printf("\t-S bit \t\tUse stop bits (1, 2). Default: 1\n");
    printf("\t-r baud_rate \tSet baud_rate meter speed (1200, 2400, 4800, 9600)\n");
    printf("\t-R new_time \tChange rotation time for displaying values (0 - 30s) (0 = no rotation)\n");
    printf("\t-m \t\tOutput values in IEC 62056 format ID(VALUE*UNIT)\n");
    printf("\t-q \t\tOutput values in compact mode\n");
    printf("\t-z num_retries\tTry to read max num_retries times on bus before exiting\n");
    printf("\t\t\twith error. Default: 1\n");
    printf("\t-j seconds\tResponse timeout. Default: 2\n");
    printf("\t-w seconds\tTime to wait to lock serial port. (1-30) Default: 0\n");
    printf("\t-1 \t\tModel: SDM120C (default)\n");
    printf("\t-2 \t\tModel: SDM220\n");
    printf("\tdevice\t\tSerial device, i.e. /dev/ttyUSB0\n\n");
    printf("Serial device is required. When no parameter is passed, retrives all values\n");
}

/*--------------------------------------------------------------------------
    getCurTime
----------------------------------------------------------------------------*/
char* getCurTime()
{
    time_t curTimeValue;
    struct tm timStruct;
    static char CurTime[18] = " ";

    curTimeValue = time(NULL);
    timStruct = *(localtime(&curTimeValue));
    strftime(CurTime,sizeof(CurTime),"%Y%m%d-%H:%M:%S",&timStruct);
    CurTime[sizeof(CurTime)-1] = '\0';

    return CurTime;
}

void log_message(const int debug, const char* format, ...) {
    va_list args;
    if (debug)
       fprintf(stderr, "%s: ", getCurTime());
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

/*--------------------------------------------------------------------------
        getMemPtr
----------------------------------------------------------------------------*/
void *getMemPtr(size_t mSize)
{
    void *ptr;
    char *cptr;
    int i;

    ptr = malloc(mSize);
    if (!ptr) {
        log_message(debug_flag, "\nvproweather: malloc failed\n");
        exit(2);
    }
    cptr = (char *)ptr;
    for (i = 0; i < mSize; i++) cptr[i] = '\0';
    return ptr;
}

/*--------------------------------------------------------------------------
    ClrSerLock
    Clear Serial Port lock.
----------------------------------------------------------------------------*/
int ClrSerLock(long unsigned int PID) {
    FILE *fdserlck, *fdserlcknew;
    long unsigned int rPID;
    int bWrite, bRead;
    int errno_save = 0;

    errno = 0;
    if (debug_flag) log_message(debug_flag, "\ndevLCKfile: <%s>\ndevLCKfileNew: <%s>\nClearing Serial Port Lock (%lu)...", devLCKfile, devLCKfileNew, PID);
    fdserlck = fopen(devLCKfile, "r");
    if (fdserlck == NULL) {
        fprintf(stderr, "\n\n");
        log_message(debug_flag, "%s: Problem opening serial device lock file to clear PID %lu: %s for read.\n\n",programName,PID,devLCKfile);
        return(0);
    }
    fdserlcknew = fopen(devLCKfileNew, "w");
    if (fdserlcknew == NULL) {
        fprintf(stderr, "\n\n");
        log_message(debug_flag, "%s: Problem opening new serial device lock file to clear PID %lu: %s for write.\n\n",programName,PID,devLCKfileNew);
        fclose(fdserlck);
        return(0);
    }
    bRead = fscanf(fdserlck, "%lu", &rPID);
    while (bRead != EOF) {
        if (rPID != PID) {
            errno = 0;
            bWrite = fprintf(fdserlcknew, "%lu\n", rPID);
            errno_save = errno;
            if (bWrite < 0 || errno_save != 0) {
                fprintf(stderr, "\n\n");
                log_message(debug_flag, "%s: Problem clearing serial device lock, can't write lock file: %s.\n%s\n\n",programName,devLCKfile,strerror (errno_save));
                fclose(fdserlcknew);
                return(0);
            }
        }
        bRead = fscanf(fdserlck, "%lu", &rPID);
    }
    fclose(fdserlck);
    fclose(fdserlcknew);
    errno = 0;
    if (rename(devLCKfileNew,devLCKfile)) 
        log_message(debug_flag, "%s: Problem clearing serial device lock, can't update lock file: %s.\n%s\n\n",programName,devLCKfile,strerror (errno));
        if (debug_flag)
            fprintf(stderr, " .done\n");

    return -1;
}

inline void exit_error(modbus_t *ctx)
{
      modbus_close(ctx);
      modbus_free(ctx);
      //EXIT_ERROR++;
      ClrSerLock(PID);
      //return -1;
      printf(" NOK\n");
      exit(EXIT_FAILURE);
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

#if 0

// unused

int getMeasureBCD(modbus_t *ctx, int address, int retries, int nb) {

    uint16_t tab_reg[nb * sizeof(uint16_t)];
    int rc;
    int i;
    int j = 0;
    int exit_loop = 0;

    while (j < retries && exit_loop == 0) {
      j++;

      if (debug_flag == 1) {
         log_message(debug_flag, "%d/%d. Register Address %d [%04X]\n", j, retries, 30000+address+1, address);
      }
      rc = modbus_read_input_registers(ctx, address, nb, tab_reg);

      if (rc == -1) {
        log_message(debug_flag, "%s: ERROR %s, %d\n", programName, modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }

    if (rc == -1) {
      exit_error(ctx);
    }

    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          log_message(debug_flag, "reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
       }
    }

    int value = bcd2num(&tab_reg[0], rc);

    return value;
}

#endif

float getMeasureFloat(modbus_t *ctx, int address, int retries, int nb) {

    uint16_t tab_reg[nb * sizeof(uint16_t)];
    int rc;
    int i;
    int j = 0;
    int exit_loop = 0;

    while (j < retries && exit_loop == 0) {
      j++;

      if (debug_flag == 1) {
         log_message(debug_flag, "%d/%d. Register Address %d [%04X]\n", j, retries, 30000+address+1, address); 
      }
      rc = modbus_read_input_registers(ctx, address, nb, tab_reg);

      if (rc == -1) {
        log_message(debug_flag, "%s: ERROR %s, %d\n", programName, modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }

    if (rc == -1) {
      exit_error(ctx);
    }

    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          log_message(debug_flag, "reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
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
         log_message(debug_flag, "%d/%d. Register Address %d [%04X]\n", j, retries, 400000+address+1, address);
      }
      rc = modbus_read_registers(ctx, address, nb, tab_reg);

      if (rc == -1) {
        log_message(debug_flag, "%s: ERROR %s, %d\n", programName, modbus_strerror(errno), j);
        modbus_flush(ctx);
        usleep(2500);
      } else {
        exit_loop = 1;
      }
    }

    if (rc == -1) {
      exit_error(ctx);
    }

    if (debug_flag == 1) {
       for (i=0; i < rc; i++) {
          log_message(debug_flag, "reg[%d/%d]=%d (0x%X)\n", i, (rc-1), tab_reg[i], tab_reg[i]);
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
        printf("%s: errno: %s, %d, %d\n", programName, modbus_strerror(errno), errno, n);
        exit_error(ctx);
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
        log_message(debug_flag, "%s: errno: %s, %d, %d\n", programName, modbus_strerror(errno), errno, n);
        exit_error(ctx);
    }
}

void lockSer(const char *szttyDevice, int debug_flag)
{
    char *pos;
    FILE *fdserlck = NULL;
    long unsigned int rPID;
    char sPID[10];
    int bRead, bWrite, lckCNT;
    int errno_save = 0;
    int fLen = 0;
    char *cmdFile = NULL;
    char *command = NULL;
    char *SubStrPos = NULL;

    pos = strrchr(szttyDevice, '/');
    if (pos > 0) {
        pos++;
        devLCKfile = getMemPtr(strlen(ttyLCKloc)+(strlen(szttyDevice)-(pos-szttyDevice))+1);
        devLCKfile[0] = '\0';
        strcpy(devLCKfile,ttyLCKloc);
        strcat(devLCKfile, pos);
        devLCKfile[strlen(devLCKfile)] = '\0';
        sprintf(sPID,"%lu",PID);
        devLCKfileNew = getMemPtr(strlen(devLCKfile)+strlen(sPID)+2);	/* dot & terminator */
        devLCKfileNew[0] = '\0';
        strcpy(devLCKfileNew,devLCKfile);
        sprintf(devLCKfileNew,"%s.%lu",devLCKfile,PID);
        devLCKfileNew[strlen(devLCKfileNew)] = '\0';
    } else {
        devLCKfile = NULL;
    }

    if (debug_flag) {
        log_message(debug_flag, "\nszttyDevice: %s",szttyDevice);
        log_message(debug_flag, "\ndevLCKfile: <%s>\ndevLCKfileNew: <%s>\n",devLCKfile,devLCKfileNew);
    }

    if (debug_flag) log_message(debug_flag, "\nAttempting to get lock on Serial Port %s...\n",szttyDevice);
    fdserlck = fopen((const char *)devLCKfile, "a");
    if (fdserlck == NULL) {
        if (debug_flag) fprintf(stderr, "\n");
        log_message(debug_flag, "%s: Problem locking serial device, can't open lock file: %s for write.\n\n",programName,devLCKfile);
        exit(2);
    }
    bWrite = fprintf(fdserlck, "%lu\n", PID);
    errno_save = errno;
    fclose(fdserlck);
    fdserlck = NULL;
    if (bWrite < 0 || errno_save != 0) {
        if (debug_flag) log_message(debug_flag, "\n");
        log_message(debug_flag, "%s: Problem locking serial device, can't write lock file: %s.\n%s\n\n",programName,devLCKfile,strerror (errno_save));
        exit(2);
    }

    rPID = 0;
    lckCNT = -1;
    while(rPID != PID && lckCNT++ < yLockWait) {
        if (debug_flag && lckCNT == 0) log_message(debug_flag, "Checking for lock\n");
        SubStrPos = NULL;
        fdserlck = fopen(devLCKfile, "r");
        if (fdserlck == NULL) {
            if (debug_flag) log_message(debug_flag, "\n");
            log_message(debug_flag, "%s: Problem locking serial device, can't open lock file: %s for read.\n\n",programName,devLCKfile);
            exit(2);
        }
        bRead = fscanf(fdserlck, "%lu", &rPID);
        errno_save = errno;
        fclose(fdserlck);
        if (debug_flag) log_message(debug_flag, "\nChecking process %lu for lock\n",rPID);
        fdserlck = NULL;
        if (bRead == EOF || errno_save != 0) {
            if (debug_flag) log_message(debug_flag, "\n");
            log_message(debug_flag, "%s: Problem locking serial device, can't read lock file: %s.\n%s\n\n",programName,devLCKfile,strerror (errno_save));
            exit(2);
        }

        sPID[0] = '\0';
        sprintf(sPID,"%lu",rPID);
        cmdFile = getMemPtr(strlen(sPID)+14+1);
        cmdFile[0] = '\0';
        sprintf(cmdFile,"/proc/%lu/cmdline",rPID);
        cmdFile[strlen(cmdFile)] = '\0';
        if (debug_flag) log_message(debug_flag, "cmdFile=\"%s\"\n", cmdFile);
        fdserlck = fopen(cmdFile, "r");
        if (fdserlck != NULL) {
            fLen = 0;
            while (fgetc(fdserlck) != EOF) fLen++;
            if (fLen > 0) {
                command = getMemPtr(fLen+1);
                command[0] = '\0';
                rewind(fdserlck);
                bRead = fscanf(fdserlck, "%s", command);
                command[strlen(command)] = '\0';
                if (debug_flag) log_message(debug_flag, "command=\"%s\"\n", command);
            }
            fclose(fdserlck);
            fdserlck = NULL;
            if (command != NULL) SubStrPos = strstr(command, programName);
        }
        if (cmdFile != NULL) {
            free(cmdFile);
            cmdFile = NULL;
        }
        if (debug_flag) {
            fprintf (stderr, "rPID: %lu SubStrPos: %s command: %s",rPID,SubStrPos,command);
            if (rPID == PID) fprintf (stderr, " = me");
            fprintf (stderr, "\n");
        }
        if (rPID != PID) {
             if (command == NULL) {          // Clear stale only if rPID process is dead (Aurora <= 1.8.8 needs a patch too)
                 if (debug_flag) fprintf (stderr, "\n");
                 log_message(debug_flag, "%s: Clearing stale serial port lock. (%lu)\n",programName,rPID);
                 ClrSerLock(rPID);
             } else if (yLockWait > 0)
                 sleep(1);
        }
        if (command != NULL) {
            free(command);
            command = NULL;
        }
    }
    if (debug_flag && rPID == PID) log_message(debug_flag, "Appears we got the lock.\n");
    if (rPID != PID) {
        ClrSerLock(PID);
        if (debug_flag) fprintf (stderr, "\n");
        log_message(debug_flag, "%s: Problem locking serial device %s, couldn't get the lock for %lu, locked by %lu.\n\n",programName,szttyDevice,PID,rPID);
        exit(2);
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
    char *szttyDevice  = NULL;
    char *c_parity     = NULL;
    int speed          = 0;
    int bits           = 0;
    int read_count     = 0;

    const char *EVEN_parity = "E";
    const char *NONE_parity = "N";
    const char *ODD_parity  = "O";
    char parity             = E_PARITY;
    programName       = argv[0];

    char *pos;
    FILE *fdserlck = NULL;
    long unsigned int rPID;
    char sPID[10];
    int bRead, bWrite, lckCNT;
    int errno_save = 0;
    int fLen = 0;
    char *cmdFile = NULL;
    char *command = NULL;
    char *SubStrPos = NULL;

    if (argc == 1) {
        usage(programName);
        exit(EXIT_FAILURE);
    }

    opterr = 0;

    while ((c = getopt (argc, argv, "a:b:cdefgij:lmnpP:qr:R:s:S:tTvw:xz:12")) != -1) {
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
            case 'x':
                trace_flag = 1;
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
                c_parity = strdup(optarg);
                if (strcmp(c_parity,EVEN_parity) == 0) {
                    parity = E_PARITY;
                } else if (strcmp(c_parity,NONE_parity) == 0) {
                    parity = N_PARITY;
                } else if (strcmp(c_parity,ODD_parity) == 0) {
                    parity = O_PARITY;
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
            case 'w':
                yLockWait = atoi(optarg);
                if (yLockWait < 1 || yLockWait > 30) {
                    log_message(debug_flag, "%s: -w Lock Wait seconds (%d) out of range, 1-30.\n",programName,yLockWait);
                    return 0;
                }
                break;
            case 'T':
                time_disp_flag = 1;
                count_param++;
                break;
            case '?':
                if (optopt == 'a' || optopt == 'b' || optopt == 's') {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                    usage(programName);
                    exit(EXIT_FAILURE);
                }
                else if (isprint (optopt)) {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                    usage(programName);
                    exit(EXIT_FAILURE);
                }
                else {
                    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                    usage(programName);
                    exit(EXIT_FAILURE);
                }
            default:
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                usage(programName);
                exit(EXIT_FAILURE);
        }
    }

#if 0
    if ((optind+1) > argc) {
        /* need at least one argument (change +1 to +2 for two, etc. as needeed) */
        if (debug_flag == 1) {
           log_message(debug_flag, "optind = %d, argc = %d\n", optind, argc);
        }
        usage(programName);
        exit(EXIT_FAILURE);
    }
    pos = strrchr(szttyDevice, '/');
    if (pos > 0) {
        pos++;
        devLCKfile = getMemPtr(strlen(ttyLCKloc)+(strlen(szttyDevice)-(pos-szttyDevice))+1);
        devLCKfile[0] = '\0';
        strcpy(devLCKfile,ttyLCKloc);
        strcat(devLCKfile, pos);
        devLCKfile[strlen(devLCKfile)] = '\0';
        sprintf(sPID,"%lu",PID);
        devLCKfileNew = getMemPtr(strlen(devLCKfile)+strlen(sPID)+2);	/* dot & terminator */
        devLCKfileNew[0] = '\0';
        strcpy(devLCKfileNew,devLCKfile);
        sprintf(devLCKfileNew,"%s.%lu",devLCKfile,PID);
        devLCKfileNew[strlen(devLCKfileNew)] = '\0';
    } else {
    }
#endif

    if (optind < argc) {               /* get serial device name */
        szttyDevice = argv[optind];
     } else {
        /* need at least one argument (change +1 to +2 for two, etc. as needeed) */
        if (debug_flag == 1) {
           log_message(debug_flag, "optind = %d, argc = %d\n", optind, argc);
        }
        usage(programName);
        log_message(debug_flag, "%s: No serial device specified\n",programName);
        exit(EXIT_FAILURE);
    }


    if (compact_flag == 1 && metern_flag == 1) {
        log_message(debug_flag, "Parameter -m and -q are mutually exclusive\n\n");
        usage(programName);
        exit(EXIT_FAILURE);
    }

    PID = getpid();
    lockSer(szttyDevice, debug_flag);

    modbus_t *ctx;
    if (baud_rate == 0) baud_rate = DEFAULT_RATE;

    ctx = modbus_new_rtu(szttyDevice, baud_rate, parity, 8, stop_bits);

    if (ctx == NULL) {
        log_message(debug_flag, "Unable to create the libmodbus context\n");
        ClrSerLock(PID);
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

    if (trace_flag == 1) {
        modbus_set_debug(ctx, 1);
    }

    int slave = 0;
    slave = modbus_set_slave(ctx, device_address);

    if (modbus_connect(ctx) == -1) {
        log_message(debug_flag, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        ClrSerLock(PID);
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

        log_message(debug_flag, "Parameter -s and -r are mutually exclusive\n\n");
        usage(programName);
        exit_error(ctx);
    } else if (new_address > 0) {

        if (count_param > 0) {
            usage(programName);
            modbus_close(ctx);
            modbus_free(ctx);
            ClrSerLock(PID);
            exit(EXIT_FAILURE);
        } else {
            // change Address
            changeConfigFloat(ctx, DEVICE_ID, new_address, RESTART_TRUE, 2);
            modbus_close(ctx);
            modbus_free(ctx);
            ClrSerLock(PID);
            return 0;
        }

    } else if (new_baud_rate > 0) {

        if (count_param > 0) {
            usage(programName);
            modbus_close(ctx);
            modbus_free(ctx);
            ClrSerLock(PID);
            exit(EXIT_FAILURE);
        } else {
            // change Baud Rate
            changeConfigFloat(ctx, BAUD_RATE, new_baud_rate, RESTART_TRUE, 2);
            modbus_close(ctx);
            modbus_free(ctx);
            ClrSerLock(PID);
            return 0;
        }

    } else if (rotation_time_flag > 0) {

        if (count_param > 0) {
            usage(programName);
            modbus_close(ctx);
            modbus_free(ctx);
            ClrSerLock(PID);
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
            ClrSerLock(PID);
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
        modbus_close(ctx);
        modbus_free(ctx);
        ClrSerLock(PID);
        printf(" OK\n");
    } else {
        exit_error(ctx);
    }

    /*
    if (compact_flag == 1) {
        printf("\n");
    }
    */

    return 0;
}

#ifdef __cplusplus
}
#endif
