#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <pocketsphinx.h>
#include "NiFpga.h"
#include "MyRio.h"
#include "TimerIRQ.h"

#if !defined(LoopDuration)
#define LoopDuration	60	/* How long to monitor the signal, in seconds*/
#endif

#if !defined(LoopSteps)
#define LoopSteps	3	/* How long to step between printing, in seconds */
#endif

/*
 * Resources for the new thread.
 */
typedef struct {
	NiFpga_IrqContext irqContext;	/* IRQ context reserved by Irq_ReserveContext() */
	NiFpga_Bool irqThreadRdy;	/* IRQ thread ready flag */
} ThreadResource;

/*
 * timer irq thread to poll the analog inputs
 *
 * Arguments:
 * void* resource - pointer to the thread resource
 *
 * Outputs:
 * NULL
 */
void *Timer_Irq_Thread(void* resource) {
	ThreadResource* threadResource = (ThreadResource*) resource;
	while (1) {
		uint32_t irqAssert = 0
		static uint32_t

	}

}
int main(int argc, char *argv[]) {
	ps_decoder_t *ps = NULL;
	cmd_ln_t *config = NULL;

	config = cmd_ln_init(NULL, ps_args(), TRUE, NULL);
	printf("hello");
	return 0;
}
