
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility/stw_timer.h"


/*
 * used to verify wheel pointers. Nothing special about
 * the value, other than to help identify a valid stw pointer
 */
#define MAGIC_TAG         ( 0x0FEDCA3BA )


/*
 * Name
 *     tmr_enqueue
 *
 *  Description:
 *     Enqueues the timer to the proper spoke per delay.
 *
 *  Parameters:
 *     *stw             pointer to the timer wheel that the timer
 *                      will run on
 *
 *     *tmr             pointer to the timer element
 *
 *     delay            delay in milliseconds
 *
 *  Returns:
 *     Nothing
 *
 */
void CSTWTimer::tmr_enqueue (stw_t *stw, stw_tmr_t *tmr, uint32 delay)
{
    stw_links_t  *prev, *spoke;

    uint32 cursor;
    uint32 ticks;
    uint32 td;

    if (delay < stw->granularity) {
        /*
         * must delay at least one tick, can not delay in past...
         */
        ticks = 1;

    } else {
        /*
         * compute number ticks reqquired to expire the duration
         */
        ticks = (delay / stw->granularity);
    }

    /*
     * tick displacement from current cursor
     */
    td = (ticks % stw->wheel_size);

    /*
     * times around the wheel required to expire duration
     */
    tmr->rotation_count = (ticks / stw->wheel_size);

    /*
     * calculate cursor to place the timer
     */
    cursor = ((stw->spoke_index + td) % stw->wheel_size);

    spoke = &stw->spokes[cursor];
    /*
     * We have a timer and now we have a spoke.  All that is left is to
     * link the timer to the spoke's list of timers.  With a doubly linked
     * list, there is no need to check for an empty list.  We simply link
     * it to the end of the list.  This is the same price as putting it
     * on the front of the list but feels more 'right'.
     */
    prev = spoke->stw_prev;
    tmr->links.stw_next = spoke;      /* append to end of spoke  */
    tmr->links.stw_prev = prev;

    prev->stw_next   = (stw_links_t *)tmr;
    spoke->stw_prev = (stw_links_t *)tmr;


    return;
}



/*
 *+
 * Name
 *     stw_timer_stats
 *
 * Description
 *     Displays the stats for the specified timer wheel.
 *
 * Parameter
 *     *stw          pointer to the timer wheel
 *
 * Returns
 *     none
 *- 
 */
void CSTWTimer::stw_timer_stats (const stw_t *stw)
{
    /*
     * protect against a bad wheel
     */
    if ((stw == NULL) || (stw->magic_tag != MAGIC_TAG)) {
        return;
    }

    printf("\n%s \n", stw->wheel_name );
    printf("       Granularity=%u\n", stw->granularity);
    printf("        Wheel Size=%u\n", stw->wheel_size);
    printf("        Tick Count=%u\n", stw->ticks);
    printf("       Spoke Index=%u\n", stw->spoke_index);

    printf("     Active timers=%u\n", stw->timer_active);
    printf("    Expired timers=%u\n", stw->timer_expired);
    printf("      Hiwater mark=%u\n", stw->timer_hiwater_mark);
    printf("    Started timers=%u\n", stw->timer_starts);
    printf("  Cancelled timers=%u\n", stw->timer_cancelled);
    return;
}


/*
 *+ 
 * Name
 *     stw_timer_running
 *
 * Description
 *     Returns TRUE if the timer structure is active
 *
 * Parameter
 *     *tmr          pointer to the timer
 *
 * Returns
 *      TRUE==timer is active
 *     FALSE==timer is not active
 *- 
 */
uint8 CSTWTimer::stw_timer_running (stw_tmr_t *tmr)
{
    if (tmr == NULL) {
        return 0;
    }

    /*
     * if the timer is linked, its active
     */
    if (tmr->links.stw_next != NULL) {
        return 1;
    }

    return 0;
}


/*
 *+ 
 * Name
 *     stw_timer_prepare
 *
 * Descriiption
 *     Utility routine to initialize the links of timer elements.
 *
 * Parameter
 *     *tmr          pointer to the timer
 *
 * Returns
 *     none
 *- 
 */
void CSTWTimer::stw_timer_prepare (stw_tmr_t *tmr)
{
    if (tmr) {
        tmr->links.stw_next = NULL;
        tmr->links.stw_prev = NULL;
    }
}


/*
 *+ 
 * Name
 *     stw_timer_start
 *
 * Description
 *     Start (or restart) a timer to expire in the future.  If the timer is
 *     currently linked to a timer wheel spoke, it will be removed
 *     first.  A new timer spoke will be determined based on the millisecond
 *     time delay requested and linked into the appropriate
 *     timer wheel spoke.
 *
 * Parameters
 *     *stw             pointer to the timer wheel
 *
 *     *tmr             pointer to the timer element
 *
 *     delay            initial delay in milliseconds
 *
 *     periodic_delay   periodic delay in milliseconds
 *
 *     user_cb          application function call-back to be
 *                      invoked when the timer expires.  This call-back
 *                      must run-to-completion and not block.
 *
 *     parm             persistent parameter passed to the application
 *                      call-back upon expiration
 *
 * Returns
 *     RC_STW_OK
 *     error otherwise
 *- 
 */
RC_STW_t CSTWTimer::stw_timer_start( stw_t      *stw,
                                     stw_tmr_t  *tmr,
                                     uint32      delay,
                                     uint32      periodic_delay,
                                     stw_call_back   user_cb,
                                     void  *parm)
{
    stw_links_t *next, *prev;


    if (stw == NULL) {
        return (RC_STW_NULL_WHEEL);
    }

    if (tmr == NULL) {
        return (RC_STW_NULL_TMR);
    }

    if (stw->magic_tag != MAGIC_TAG) {
       return (RC_STW_INVALID_WHEEL);
    }



    /*
     * First check to see if it is already running. If so, remove
     * it from the wheel.  We don't bother cleaning up the fields
     * because we will be setting them below.
     */
    next = tmr->links.stw_next;
    if (next) {
        prev = tmr->links.stw_prev;
        next->stw_prev = prev;
        prev->stw_next = next;

        /*
         * stats book keeping
         */
        stw->timer_active--;
    }



    /*
     * set user call_back and parameter
     */
    tmr->func_ptr       = (void*)user_cb;
    tmr->parm           = parm;
    tmr->delay          = delay;
    tmr->periodic_delay = periodic_delay;

    tmr_enqueue(stw, tmr, delay);


    stw->timer_starts++;
    stw->timer_active++;
    if (stw->timer_active > stw->timer_hiwater_mark) {
        stw->timer_hiwater_mark = stw->timer_active;
    }

    return (RC_STW_OK);
}


/*
 *+ 
 * Name
 *     stw_timer_stop
 *
 * Description
 *     Stop the timer by removing it from whatever timer wheel spoke list
 *     it is currently attached to (if any).  Note that it is safe to call
 *     this function with a timer that has already been stopped in order to
 *     avoid making all callers check for a running timer first.
 *
 * Parameters
 *     *stw             pointer to the timer wheel
 *
 *     *tmr             pointer to the timer element
 *
 * Returns
 *     RC_STW_OK
 *     error otherwise
 *- 
 */
RC_STW_t CSTWTimer::stw_timer_stop (stw_t *stw, stw_tmr_t *tmr)
{
    stw_links_t *next, *prev;

    if (stw == NULL) {
        return (RC_STW_NULL_WHEEL);
    }

    if (tmr == NULL) {
        return (RC_STW_NULL_TMR);
    }

    if (stw->magic_tag != MAGIC_TAG ) {
        return (RC_STW_INVALID_WHEEL);
    }


    next = tmr->links.stw_next;
    if (next) {
        prev = tmr->links.stw_prev;
        next->stw_prev = prev;
        prev->stw_next = next;
        tmr->links.stw_next = NULL;    /* NULL == tmr is free */
        tmr->links.stw_prev = NULL;

        /*
         * stats bookkeeping
         */
        stw->timer_active--;
        stw->timer_cancelled++;
    }

    return (RC_STW_OK);
}


/*
 *+ 
 * Name
 *     stw_timer_tick
 *
 * Description
 *     Move the wheel cursor forward to the next spoke
 *     and expire the timers.
 *
 *     Application call-backs must be non-blocking
 *
 * Parameters
 *     *stw             pointer to the timer wheel
 *
 * Returns
 *     none
 *-
 */
void CSTWTimer::stw_timer_tick (stw_t *stw)
{
    stw_links_t  *spoke, *next, *prev;
    stw_tmr_t *tmr;

    stw_call_back user_call_back;

    if ((stw == NULL) || (stw->magic_tag != MAGIC_TAG)) {
        return;
    }


    /*
     * keep track of rolling the wheel
     */
    stw->ticks++;

    /*
     * advance the index to the next spoke
     */
    stw->spoke_index = ( stw->spoke_index + 1 ) %
                                stw->wheel_size;

    /*
     * Process the spoke, removing timers that have expired.
     * If the timer rotation count is positive
     * decrement and catch the timer on the next wheel revolution.
     */
    spoke = &stw->spokes[stw->spoke_index];
    tmr = (stw_tmr_t *)spoke->stw_next;

    while( (stw_links_t *)tmr != spoke) {

        next = (stw_links_t *)tmr->links.stw_next;
        prev = (stw_links_t *)tmr->links.stw_prev;

        /*
         * if the timer is a long one and requires one or more rotations
         * decrement rotation count and leave for next turn.
         */
        if (tmr->rotation_count != 0) {
            tmr->rotation_count--;
        } else {


            prev->stw_next = next;
            next->stw_prev = prev;

            tmr->links.stw_next = NULL;
            tmr->links.stw_prev = NULL;

            /* book keeping */
            stw->timer_active--;
            stw->timer_expired++;

            /*
             * Invoke the user expiration handler to do the actual work.
             */
            user_call_back = (stw_call_back)tmr->func_ptr;

            (*user_call_back)(tmr, tmr->parm);

            /*
             * automatically restart the timer if periodic_delay > 0
             */
            if (tmr->periodic_delay > 0) {
                 tmr_enqueue(stw, tmr, tmr->periodic_delay);
                 stw->timer_active++;
            }
        }

        tmr = (stw_tmr_t *)next;
    }
    return;
}


/*
 *+ 
 * Name
 *     stw_timer_destroy
 *
 * Description
 *     Destroys the specified timer wheel.  All
 *     timers are stopped and resources released.
 *
 * Parmeters
 *     stw           timer wheel
 *
 * Returns
 *     RC_STW_OK
 *     error otherwise
 *- 
 */
RC_STW_t CSTWTimer::stw_timer_destroy (stw_t *stw)
{
    uint32  j;
    stw_links_t *spoke;

    stw_tmr_t *tmr;


    if (stw == NULL) {
        return (RC_STW_NULL_WHEEL);
    }

    if (stw->magic_tag != MAGIC_TAG ) {
        return (RC_STW_INVALID_WHEEL);
    }


    /*
     * Drive around the wheel and stop all timers
     */

    for (j = 0; j < stw->wheel_size; j++) {
        spoke = &stw->spokes[j];

        tmr = (stw_tmr_t *)spoke->stw_next;

        while ( (stw_links_t *)tmr != spoke) {
            stw_timer_stop(stw, tmr);

            tmr = (stw_tmr_t *)spoke->stw_next;
        } /* end while */

    } /* end for */

    /*
     * clear the magic so we do not mistakenly access this wheel
     */
    stw->magic_tag = 0;

    /*
     * now free the wheel structures
     */
    free(stw->spokes);
    free(stw);

    return (RC_STW_OK);
}


/*
 *+ 
 * Name
 *     stw_timer_create
 *
 * Description
 *     Initializes a timer wheel.  Timers must not be started before
 *     this routine is called.
 *
 * Parmeters
 *     wheel_size      number of spokes in the wheel.  The number
 *                     of spokes should be engineered such that
 *                     wheel_size >= (longest duration / granularity )
 *                     Depending upon the number of concurrent timers, the
 *                     distribution of those timers, it may be beneficial to
 *                     further increase the wheel size.  Objective is to
 *                     minimize frequency of 'long' timers requiring wheel
 *                     revolutions.
 *
 *     granularity     milliseconds between ticks
 *
 *     *p2name         pointer to the name of identify wheel.  Limited
 *                     to STW_NAME_LENGTH
 *
 *     **stw           returned pointer to the created timer wheel
 *
 * Returns
 *     RC_STW_OK
 *     error otherwise
 *-  
 */
RC_STW_t CSTWTimer::stw_timer_create (uint32 wheel_size,
                                      uint32 granularity,
                                      const char *p2name,
                                      stw_t **stw)
{
    uint32 j;
    stw_links_t *spoke;

    stw_t  *p2stw;


    if (stw == NULL) {
        return (RC_STW_NULL_WHEEL);
    }
    *stw = NULL;

    /*
     * we need to put some bounds to protect against extremely
     * large numbers
     */
    if (wheel_size < STW_MIN_WHEEL_SIZE || wheel_size > STW_MAX_WHEEL_SIZE) {
        return (RC_STW_INVALID_WHEEL_SIZE);
    }

    if (granularity < STW_MIN_GRANULARITY ||
                      granularity > STW_MAX_GRANULARITY) {
        return (RC_STW_INVALID_GRANULARITY);
    }

    /*
     * allocate memory for wheel control structure
     */
    //p2stw = (stw_t *)malloc( sizeof(stw_header_t) +
    //                      (wheel_size * sizeof(stw_links_t)) );
    p2stw = (stw_t *)malloc( sizeof(stw_t));
    if (p2stw == NULL) {
        return (RC_STW_NO_RESOURCES);
    }

    /*
     * allocate memory for wheel spokes
     */
    p2stw->spokes = (stw_links_t *)malloc(wheel_size * sizeof(stw_links_t));
    if (p2stw->spokes == NULL) {
        free(p2stw); 
        return (RC_STW_NO_RESOURCES);
    }


    /*
     * Initialize the internal tick count at zero, should use
     * safe string lib! 
     */
    strncpy(p2stw->wheel_name, p2name, STW_NAME_LENGTH-1);
    p2stw->wheel_name[STW_NAME_LENGTH-1] = '\0';  

    p2stw->magic_tag = MAGIC_TAG;

    p2stw->ticks = 0;

    p2stw->spoke_index = 0;
    p2stw->granularity = granularity;
    p2stw->wheel_size  = wheel_size;

    /*
     * timer stats to tune wheel
     */
    p2stw->timer_hiwater_mark  = 0;
    p2stw->timer_active = 0;
    p2stw->timer_cancelled=0;
    p2stw->timer_expired=0;
    p2stw->timer_starts=0;

    /*
     * Set all spokes to empty
     */
    spoke = &p2stw->spokes[0];
    for (j = 0; j < wheel_size; j++) {
        spoke->stw_next = spoke;    /* empty spoke points to itself */
        spoke->stw_prev = spoke;
        spoke++;
    }

    *stw = p2stw;
    return (RC_STW_OK);
}


