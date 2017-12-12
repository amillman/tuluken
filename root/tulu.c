/*
 * Copyright (c) 2015,
 * National Instruments.
 * All rights reserved.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include <sys/select.h>

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <pocketsphinx.h>

#include "TimerIRQ.h"
#include "AIO.h"
#include "MyRio.h"

/*
 * Resources for the new thread.
 */
typedef struct
{
    NiFpga_IrqContext irqContext;      /* IRQ context reserved by Irq_ReserveContext() */
    NiFpga_Bool irqThreadRdy;           /* IRQ thread ready flag */
} ThreadResource;

NiFpga_Session myrio_session;
const uint32_t timeoutValue = 63;   /* in us */
MyRio_Aio A3;
volatile int16 a3_buf[2048];
volatile uint32_t a3_buf_ready = 0;

static ps_decoder_t *ps;
static cmd_ln_t *config;

#if HI == -LO
short quantize(double x) {
    return (int16) (x < 0 ? (x/Q - 0.5) : (x/Q + 0.5));
}
#elif LO == 0
int16_t quantize(double x) {
    return (int16) (x/Q);
}
#endif

static void sleep_msec(int32 ms) {
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
}

void *Timer_Irq_Thread(void* resource)
{
    ThreadResource* threadResource = (ThreadResource*) resource;
    uint16_t index = 0;

    while (1)
    {
        uint32_t irqAssert = 0;

        /*
         * Stop the calling thread, wait until a selected IRQ is asserted.
         */
        Irq_Wait(threadResource->irqContext, TIMERIRQNO,
                 &irqAssert, (NiFpga_Bool*) &(threadResource->irqThreadRdy));

        /*
         * If an IRQ was asserted.
         */
        if (irqAssert & (1 << TIMERIRQNO))
        {
            /*timer interrupt*/
            if (!a3_buf_ready) {
                a3_buf[index++] = quantize(Aio_Read(&A3));
                if (index == 2048) {
                    a3_buf_ready = 1;
                    index = 0;
                }
            }
            /*
             * Acknowledge the IRQ(s) when assertion is done.
             */
            Irq_Acknowledge(irqAssert);
            NiFpga_WriteU32(myrio_session, IRQTIMERWRITE, timeoutValue);
            NiFpga_WriteBool(myrio_session, IRQTIMERSETTIME, NiFpga_True);
        }

        /*
         * Check the indicator to see if the new thread is stopped.
         */
        if (!(threadResource->irqThreadRdy))
        {
            printf("The IRQ thread ends.\n");
            break;
        }
    }

    /*
     * Exit the new thread.
     */
    pthread_exit(NULL);

    return NULL;
}


int main(int argc, char **argv)
{
    NiFpga_Status  status;

    MyRio_IrqTimer irqTimer0;
    ThreadResource irqThread0;

    pthread_t thread;

    uint8_t utt_started, in_speech;
    int32_t k;
    char const *hyp;

    A3.val = AIA_3VAL;
    A3.wght = AIA_3WGHT;
    A3.ofst = AIA_3OFST;
    A3.is_signed = NiFpga_False;

    /* ps init */
    config = cmd_ln_init(NULL, ps_args(), 1,
        "-hmm", HMMDIR,
        "-dict", DICTFILE,
        "-kws", KWSFILE,
        NULL);

    if (config == NULL) {
        printf("FAILED TO CREATE PS CONFIG.");
        cmd_ln_free_r(config);
        return 1;
    }

    ps = ps_init(config);
    if (ps == NULL) {
        printf("FAILED TO CREATE PS DECODER.");
        cmd_ln_free_r(config);
        return 1;
    }

    /*
     * Specify the registers that correspond to the IRQ channel
     * that you need to access.
     */
    irqTimer0.timerWrite = IRQTIMERWRITE;
    irqTimer0.timerSet = IRQTIMERSETTIME;

    /*
     * Open the myRIO NiFpga Session.
     * You must use this function before using all the other functions. 
     * After you finish using this function, the NI myRIO target is ready to be used.
     */
    status = MyRio_Open();
    if (MyRio_IsNotSuccess(status))
    {
        return status;
    }

    /*
     * Read the scaling factors
     */
    Aio_Scaling(&A3);

    /*
     * Configure the timer IRQ. The Time IRQ occurs only once after Timer_IrqConfigure().
     * Return a status message to indicate if the configuration is successful. The error code is defined in IRQConfigure.h.
     */
    status = Irq_RegisterTimerIrq(&irqTimer0, &irqThread0.irqContext,
                                          timeoutValue);

    /*
     * Terminate the process if it is unsuccessful.
     */
    if (status != NiMyrio_Status_Success)
    {
        printf("CONFIGURE ERROR: %d, Configuration of Timer IRQ failed.", 
            status);

        return status;
    }

    /*
     * Set the indicator to allow the new thread.
     */
    irqThread0.irqThreadRdy = NiFpga_True;

    /*
     * Create new threads to catch the specified IRQ numbers.
     * Different IRQs should have different corresponding threads.
     */
    status = pthread_create(&thread, NULL, Timer_Irq_Thread, &irqThread0);
    if (status != NiMyrio_Status_Success)
    {
        printf("CONFIGURE ERROR: %d, Failed to create a new thread!", 
            status);

        return status;
    }

    /* expect utterance to start */
    if (ps_start_utt(ps) < 0) {
        printf("FAILED TO START UTTERANCE\n");
        return 1;
    }
    utt_started = 0;

    while (1)
    {
        if (a3_buf_ready)
        {
            ps_process_raw(ps, a3_buf, 2048/2, FALSE, FALSE);
            in_speech = ps_get_in_speech(ps);
            printf("%i\n", ps_get_in_speech(ps));
            if (in_speech && !utt_started) {
                utt_started = 1;
                /* listening... */
                printf("listening...\n");
            }
            if (!in_speech && utt_started) {
                ps_end_utt(ps);
                hyp = ps_get_hyp(ps, NULL);
                if (hyp != NULL) {
                    printf("%s\n", hyp);
                    fflush(stdout);
                }

                ps_start_utt(ps);
                utt_started = 0;
            }
            sleep_msec(100);

            a3_buf_ready = 0;
        }
    }

    /*
     * Set the indicator to end the new thread.
     */
    irqThread0.irqThreadRdy = NiFpga_False;

    /*
     * Wait for the end of the IRQ thread.
     */
    pthread_join(thread, NULL);

    /*
     * Disable timer interrupt, so you can configure this I/O next time.
     * Every IrqConfigure() function should have its corresponding clear function,
     * and their parameters should also match.
     */
    status = Irq_UnregisterTimerIrq(&irqTimer0, irqThread0.irqContext);
    if (status != NiMyrio_Status_Success)
    {
        printf("CONFIGURE ERROR: %d\n", status);
        printf("Clear configuration of Timer IRQ failed.");
        return status;
    }

    /* free ps */
    ps_free(ps);
    cmd_ln_free_r(config);

    /*
     * Close the myRIO NiFpga Session.
     * You must use this function after using all the other functions.
     */
    status = MyRio_Close();

    /*
     * Returns 0 if successful.
     */
    return status;
}
