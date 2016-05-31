#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

// From mraa example
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <mraa.h>
#define DEFAULT_IOPIN 58
int running = 0;
static int iopin;

// RT PREEMPT HOWTO WIKI
//   https://rt.wiki.kernel.org/index.php/RT_PREEMPT_HOWTO#A_Realtime_.22Hello_World.22_Example
#include <time.h>
#include <sched.h>
#include <string.h>
#define MY_PRIORITY (99)
#define MAX_SAFE_STACK (8*1024)
#define NSEC_PER_SEC    (1000000000)
void stack_prefault(void) {
  unsigned char dummy[MAX_SAFE_STACK];
  memset(dummy, 0, MAX_SAFE_STACK);
  return;
}

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("closing IO%d nicely\n", iopin);
        running = -1;
    }
}

int main(int argc, char *argv[]) {

    unsigned int i=0;
    struct timespec t;
    struct sched_param param;
    int interval = 1000000; /* 1000us*/

    param.sched_priority = MY_PRIORITY;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
      perror("sched_setscheduler failed");
      exit(-1);
    }

    mraa_result_t r = MRAA_SUCCESS;
    iopin = DEFAULT_IOPIN;

    if (argc < 2) {
        printf("Provide an int arg if you want to flash on something other than %d\n", DEFAULT_IOPIN);
    } else {
        iopin = strtol(argv[1], NULL, 10);
    }

    mraa_init();
    fprintf(stdout, "MRAA Version: %s\nStarting Blinking on IO%d\n", mraa_get_version(), iopin);

    // Setting MRAA priority fails for some reason (returns 0 instead of the set priority)
    //int p_ret=mraa_set_priority(MY_PRIORITY);
    //fprintf(stdout, "MRAA Set Priority %d\n", p_ret);

    mraa_gpio_context gpio;
    gpio = mraa_gpio_init(iopin);
    if (gpio == NULL) {
        fprintf(stderr, "Are you sure that pin%d you requested is valid on your platform?", iopin);
        exit(1);
    }
    printf("Initialised pin%d\n", iopin);

    // set direction to OUT
    r = mraa_gpio_dir(gpio, MRAA_GPIO_OUT);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
    }

    signal(SIGINT, sig_handler);

    /* Lock memory */

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
      perror("mlockall failed");
      exit(-2);
    }

    /* Pre-fault our stack */

    stack_prefault();

    clock_gettime(CLOCK_MONOTONIC ,&t);
    /* start after one second */
    t.tv_sec++;

    printf("Start toggling PIN \n");
    while(running==0) {
      /* wait until next shot */
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

      i++;
      if(i%2) {r = mraa_gpio_write(gpio, 1);}
      else {r = mraa_gpio_write(gpio, 0);}

      /* calculate next shot */
      t.tv_nsec += interval;

      while (t.tv_nsec >= NSEC_PER_SEC) {
        t.tv_nsec -= NSEC_PER_SEC;
        t.tv_sec++;
      }
    }

    r = mraa_gpio_close(gpio);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
    }

    return r;
  }
