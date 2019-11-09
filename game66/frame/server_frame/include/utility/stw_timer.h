
#ifndef __STW_TIMER_H__
#define __STW_TIMER_H__

#include "utility/basicTypes.h"

typedef enum {
    RC_STW_OK = 0,
    RC_STW_NULL_NAME,
    RC_STW_NULL_FV,
    RC_STW_NULL_WHEEL,
    RC_STW_NULL_TMR,
    RC_STW_INVALID_WHEEL,
    RC_STW_INVALID_WHEEL_SIZE,
    RC_STW_INVALID_GRANULARITY,
    RC_STW_NO_RESOURCES,
} RC_STW_t;

/*
 * range of valid wheel sizes
 */
#define STW_MIN_WHEEL_SIZE    (   32 )
#define STW_MAX_WHEEL_SIZE    ( 4096 )

/*
 * Granularity of a timer tick in milliseconds   
 */
#define STW_MIN_GRANULARITY   (   1  )
#define STW_MAX_GRANULARITY   ( 1000 )


/*
 * stw_links
 *  Definition of the pointers used to link a timer into
 *  a spoke.  Double-linked list for efficiency.
 */
typedef struct stw_links_t_ {
    struct stw_links_t_ *stw_next;
    struct stw_links_t_ *stw_prev;
} stw_links_t;



/*
 * Timer Wheel Structure used to manage the timer wheel
 * and keep stats to help understand performance
 */
#define STW_NAME_LENGTH   ( 32 )

typedef struct {
    char  wheel_name[ STW_NAME_LENGTH ];
    uint32  magic_tag;           /* for sanity check */
    uint32  wheel_size;
    uint32  spoke_index;         /* mod index around wheel */
    uint32  ticks;               /* absolute ticks */
    uint32  granularity;         /* millisecond per tick */

    /*
     * few book keeping parameters to help engineer the wheel
     */
    uint32  timer_hiwater_mark;
    uint32  timer_active;
    uint32  timer_cancelled;
    uint32  timer_expired;
    uint32  timer_starts;

    stw_links_t  *spokes;
} stw_t;


/*
 * stw_tmr_t
 *  Definition of a timer element.
 *  This can be malloc'ed or embedded into an existing
 *  application structure.
 */
typedef struct {
    stw_links_t links;
    uint32    rotation_count;
    uint32    delay;            /* initial delay       */
    uint32    periodic_delay;   /* auto-restart if > 0 */
    void       *func_ptr;
    void       *parm;
} stw_tmr_t;


/*
 * Application call-back type to be invoked at timer
 * expiration.  The call-back must be short-n-sweet,
 * non-blocking.
 */
typedef void (*stw_call_back)(stw_tmr_t *tmr, void *parm);


class CSTWTimer
{
public:
    
static void   tmr_enqueue (stw_t *stw, stw_tmr_t *tmr, uint32 delay);
/*
 * stw_timer_running
 *  Returns TRUE if the timer is active
 */ 
static uint8  stw_timer_running(stw_tmr_t *tmr);


/*
 * stw_timer_prepare
 *  Utility routine to initialize the links of timer elements.
 */
static void   stw_timer_prepare(stw_tmr_t *tmr);


/*
 * Displays timer wheel stats and counters to stdout.
 */
static void   stw_timer_stats(const stw_t *stw);


/*
 * Starts a new timer.  If the timer is currently running,
 * it is stopped and restarted anew
 */
static RC_STW_t stw_timer_start(stw_t  *stw,
                                stw_tmr_t  *tmr,
                                uint32   delay,
                                uint32   periodic_delay,
                                stw_call_back   user_cb,
                                void   *parm);


/*
 * stops a currently running timer
 */
static RC_STW_t stw_timer_stop(stw_t *stw, stw_tmr_t *tmr);


/*
 * Timer Wheel tick handler which drives time for the
 * specified wheel
 */
static void     stw_timer_tick(stw_t *stw);

/*
 * Destroys a timer wheel
 */
static RC_STW_t stw_timer_destroy(stw_t *stw);


/*
 * creates and initializes a single timer wheel
 */
static RC_STW_t stw_timer_create(uint32   wheel_size,
                                 uint32   granularity,
                                 const char  *name,
                                 stw_t **stw);

};

#endif /* __STW_TIMER_H__ */


