/* Simple Watchdog Daemon : Simple watchdog daemon to refresh hardware
 * watchdog (defautl: /dev/watchdog)
 *
 * Copyright (C) 2021  Jeremy Esquirol  jeremy.esquirol@occitaline.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <linux/watchdog.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>


/*************************************************/
#define DEFAULT_TIMEOUT         10
#define DEFAULT_PINGRATE         5
#define DEFAULT_BACKGROUND       0

/*************************************************/
static int wdogOpen(char*, int);
static int wdogPing(void);
static int wdogClose(void);
static void printUsage(char *appName);

void sigHandler(int signum, siginfo_t* info, void* ptr);
void catchSigterm(void);

/*************************************************/
static int fd = -1;
static unsigned char appRunning = 1;
static unsigned char wdogDisable = 0;
static const char *sopts = "dD:t:p:bh";
static const struct option lopts[] = {
        {"disable", no_argument, NULL, 'd'},
        {"Devname", required_argument,NULL,'D'},
        {"timeout", optional_argument, NULL, 't'},
        {"pingrate", optional_argument, NULL, 'p'},
        {"background", no_argument, NULL, 'b'},
        {"help", no_argument, NULL, 'h'},
        {NULL, no_argument, NULL, 0}
};


/**
* @name 		: main
* @brief        : main function.
**/
int main(int argc, char** argv) {
    const char* pidFileName = "/var/run/wdogd.pid";
    pid_t pid, sid;
    FILE* pidFile = NULL;
    int opt;

    /* Configurable option */
    char* devFilename = "/dev/watchdog";
    int wdogTimeout = DEFAULT_TIMEOUT;
    int pingRate = DEFAULT_PINGRATE;
    int background = DEFAULT_BACKGROUND;

    /* Enable SIGTERM handler */
    catchSigterm();

    /* Parse argument and configure watchdog */
    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
        switch (opt)
        {
            case 'd':
                wdogDisable = 1;
                break;
            case 'D':
                devFilename = optarg;
                break;
            case 't':
                if(optarg) {
                    wdogTimeout = atoi(optarg);
                } else {
                    wdogTimeout = DEFAULT_TIMEOUT;
                }
                break;
            case 'p':
                if(optarg) {
                    pingRate = atoi(optarg);
                } else {
                    pingRate = DEFAULT_PINGRATE;
                }
                break;
            case'b':
                background = 1;
                break;
            case'h':
            default:
                printUsage(argv[0]);
                return 0;
        }

    if (background) {
        /* Fork Parent Process */
        pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Error while forking the daemon");
            return 2;
        }
        if (pid > 0) /* Parent Exit */
            exit(EXIT_SUCCESS);

        /* Change file mode mask */
        umask(0);

        /* Get unique Session ID */
        sid = setsid();
        if (sid < 0) {
            syslog(LOG_ERR, "Error while getting SID");
            return 3;
        }
        /* change working directory to a safe place */
        if((chdir("/")) <0) {
            syslog(LOG_ERR,"Error while changing working directory;");
            return 4;
        }

        /* Close standard I/O */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }


    /* Opend Hardware Watchdog Driver */
    if(wdogOpen(devFilename, wdogTimeout))
        return 1;

    /* Write PID in file */
    pidFile = fopen(pidFileName,"w");
    if(pidFile != NULL) {
        fprintf(pidFile,"%d",getpid());
        fclose(pidFile);
    } else {
        syslog(LOG_WARNING, "Warning failed to write pid in %s", pidFileName);
    }

    /* Daemon Loop */
    while (appRunning) {
        wdogPing();
        sleep(pingRate);
    }

    if(remove(pidFileName))
        syslog(LOG_WARNING,"Warning failed to remove pid file : %s",pidFileName);

    return 0;
}

/**
* @name 		: printUsage
* @brief		: Print wdog help.
**/
static void printUsage(char *appName)
{
    printf("Usage: %s [options]\n", appName);
    printf("-d, --disable       Turn off watchdog timer before leaving\n");
    printf("-D, --Devname       Watchdog /dev file name (default /dev/watchdog)\n");
    printf("-t, --timeout=T     Set timeout to T seconds (default 10 seconds)\n");
    printf("-p, --pingrate=P    Set ping rate to P seconds (default 5 seconds)\n");
    printf("-b, --background    Launch %s in background\n",appName);
    printf("-h, --help          Print app usage\n");
    printf("Example : Launch %s in Background, watchdog timeout to 10 seconds, with refresh (ping rate) to 5 seconds:\n",appName);
    printf("\t %s -t=10 -p=5 -b\n",appName);
}

/**
* @name 		: wdogOpen
* @brief		: Open /dev/watchdog and set 'timeout'.
**/
static int wdogOpen(char* watchdogDriver, int timeout)
{
    struct watchdog_info id;
    FILE* dmesgFile = NULL;

    fd = open(watchdogDriver,O_WRONLY|O_CLOEXEC);
    if(fd < 0) {
        syslog(LOG_ERR,"Error while opening /dev/watchdog");
        return 1;
    }

    if(ioctl(fd, WDIOC_GETSUPPORT, &id) >=0 ) {
        syslog(LOG_INFO,"Watchdog daemon : started with '%s' driver, version %x", id.identity, id.firmware_version);
    }

    if(ioctl(fd, WDIOC_SETTIMEOUT, &timeout) >= 0 ) {
        syslog(LOG_INFO,"Watchdog timeout set to %d seconds.", timeout);
    } else {
        syslog(LOG_ERR,"WDIOC_SETTIMEOUT error '%s'", strerror(errno));
        //don't stop here, if setting timeout fail it will just be longer that specified
    }

    return 0;
}

/**
* @name 		: wdogPing
* @brief		: Feed the dog.
**/
static int wdogPing(void)
{
    if(ioctl(fd, WDIOC_KEEPALIVE, 0) < 0) {
        syslog(LOG_ERR,"Error while pinging watchdog");
        return 1;
    }

    return 0;
}
/**
 * @name 		: wdogClose
 * @brief		: close /dev/watchdog.
 *                If option "disable" was set, disable the watchdog.
 **/
static int wdogClose(void) {
    int disarmFlag = WDIOS_DISABLECARD;
    const char magicValue = 'V';

    int ret = 0;

    if (fd >= 0) {
        if (wdogDisable) {
                if (ioctl(fd, WDIOC_SETOPTIONS, &disarmFlag)) {
                    syslog(LOG_ERR, "Something went wrong while calling system to disarm watchdog");
                    ret = 1;
                }
                /* In addition to the syscall we use the "magic value" to ensure watchdog disarmement */
                if (write(fd, &magicValue, 1) < 1) {
                    syslog(LOG_ERR, "Error while disarming the watchdog. It might still be active");
                    ret = 1;
                } else {
                    ret = 0;
                }
            }
            fd = close(fd);
    }
    return ret;
}

/**
 * @name 		: sigHandler
 * @brief		: Sigterm handler. stop wdog daemon.
 **/
void sigHandler(int signum, siginfo_t* info, void* ptr)
{
    syslog(LOG_INFO,"Deamon stopped by SIGTERM");

    wdogClose();
    appRunning=0;
}
/**
 * @name 		: catchSigterm
 *  @brief		: Enable the catch of SIGTERM
 **/
void catchSigterm(void)
{
    static struct sigaction _sigact;

    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction=sigHandler;
    _sigact.sa_flags=SA_SIGINFO;

    sigaction(SIGTERM, &_sigact, NULL);
}

