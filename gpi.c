#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "beaglebone_gpio.h"

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

int main(int argc, char *argv[]) {
  volatile void *gpio_addr = NULL;
  volatile unsigned int *gpio_oe_addr = NULL;
  volatile unsigned int *gpio_setdataout_addr = NULL;
  volatile unsigned int *gpio_cleardataout_addr = NULL;
  unsigned int reg;

  unsigned int i=0;
  struct timespec t;
  struct sched_param param;
  int interval = 50000; /* 50us*/

  param.sched_priority = MY_PRIORITY;
  if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    perror("sched_setscheduler failed");
    exit(-1);
  }


  int fd = open("/dev/mem", O_RDWR);

  printf("Mapping %X - %X (size: %X)\n", GPIO1_START_ADDR,
  GPIO1_END_ADDR, GPIO1_SIZE);

  gpio_addr = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE,
    MAP_SHARED, fd, GPIO1_START_ADDR);

    gpio_oe_addr = gpio_addr + GPIO_OE;
    gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    if(gpio_addr == MAP_FAILED) {
      printf("Unable to map GPIO\n");
      exit(1);
    }
    printf("GPIO mapped to %p\n", gpio_addr);
    printf("GPIO OE mapped to %p\n", gpio_oe_addr);
    printf("GPIO SETDATAOUTADDR mapped to %p\n",
    gpio_setdataout_addr);
    printf("GPIO CLEARDATAOUT mapped to %p\n",
    gpio_cleardataout_addr);

    reg = *gpio_oe_addr;
    printf("GPIO1 configuration: %X\n", reg);
    reg = reg & (0xFFFFFFFF - PIN);
    *gpio_oe_addr = reg;
    printf("GPIO1 configuration: %X\n", reg);



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
    while(1) {
      /* wait until next shot */
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

      i++;
      if(i%2) {*gpio_setdataout_addr= PIN;}
      else {*gpio_cleardataout_addr = PIN;}

      /* calculate next shot */
      t.tv_nsec += interval;

      while (t.tv_nsec >= NSEC_PER_SEC) {
        t.tv_nsec -= NSEC_PER_SEC;
        t.tv_sec++;
      }
    }

    close(fd);
    return 0;
  }
