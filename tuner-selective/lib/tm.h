#ifndef TM_H
#define TM_H 1

#ifdef HAVE_CONFIG_H
# include "STAMP_config.h"
#endif

#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

#  include <assert.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#  include "thread.h"

#include <immintrin.h>
#include <rtmintrin.h>

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */
#  define TM_CALLABLE                   /* nothing */

#  define TM_STARTUP(numThread)         THREAD_MUTEX_INIT(the_lock); THREAD_MUTEX_INIT(aux_lock);
#  define TM_SHUTDOWN()                 { \
	}\


#  define TM_THREAD_ENTER()             /* nothing */
#  define TM_THREAD_EXIT()

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

#  define P_MALLOC(size)                malloc(size)  
#  define P_FREE(ptr)                   free(ptr)    
#  define TM_MALLOC(size)               malloc(size)  
#  define TM_FREE(ptr)                  free(ptr)    

# define STUBBORN               0
# define HALVEN                 1
# define GIVEUP                 2
# define likely(x)              __builtin_expect(!!(x), 1)
# define unlikely(x)            __builtin_expect(!!(x), 0)
# define IS_LOCKED(lock)        *((volatile int*)(&lock)) != 0

# define AL_LOCK(idx)                    /* nothing */
# define TM_BEGIN(b) { \
		current_memoized = &(memoized_blocks[b]); \
		int tries = current_memoized->retries; \
        int believedCapacityAborts = current_memoized->havingCapacityAborts; \
        unsigned long long startTicks; \
        int blockRuns = current_memoized->runs; \
        int shouldProfile = blockRuns % 100 == 0; \
		if (unlikely(shouldProfile)) { \
			startTicks = tick(); \
		} \
		current_memoized->runs++; \
        while (1) { \
            while (IS_LOCKED(the_lock)) { __asm__ ( "pause;" ); } \
            int status = _xbegin(); \
            if (status == _XBEGIN_STARTED) {  break; } \
            if (status == _XABORT_CAPACITY) { \
                current_memoized->abortsCapacity++; \
                if (believedCapacityAborts == GIVEUP) { tries = 1; } \
                else if (believedCapacityAborts == HALVEN) { tries = tries / 2 + 1; } \
            } else {\
                current_memoized->abortsTransient++; \
            } \
            tries--; \
			if (tries <= 0) {   \
				if (unlikely(shouldProfile)) { \
					double totalAborts = current_memoized->abortsTransient + current_memoized->abortsCapacity; \
					double percCapacity = current_memoized->abortsCapacity / totalAborts; \
					double percTransient = 1 - percCapacity; \
					int current_retries = current_memoized->retries; \
					if (believedCapacityAborts && current_retries > 0) { \
						current_memoized->retries = current_retries - (percCapacity * current_retries); \
					} \
					else if (believedCapacityAborts == 0 && current_retries < 16) { \
						current_memoized->retries = current_retries + (percTransient * (16-current_retries)); \
					} \
				} \
				pthread_mutex_lock(&the_lock); \
				break;  \
			}   \
        }


#    define TM_END()      if (tries > 0) {    \
                                if (IS_LOCKED(the_lock)) _xabort(30); \
                                _xend();    \
                            } else {    \
                                pthread_mutex_unlock(&the_lock);    \
                            }\
                            if (unlikely(shouldProfile)) { \
                            	unsigned long long finalTicks = tick() - startTicks; \
                            	double rewardCapacity = 0.0, rewardOther = 0.0, rewardGiveUp = 0.0 ; \
                            	if (believedCapacityAborts == STUBBORN) { \
                            	    current_memoized->cyclesTransient += finalTicks; \
                            	} else if (believedCapacityAborts == GIVEUP) { \
                            	    current_memoized->cyclesGiveUp += finalTicks; \
                            	} else { \
                            	    current_memoized->cyclesCapacity += finalTicks; \
                            	} \
                            	double logSum = 2*log((double)(current_memoized->believedCapacity + current_memoized->believedTransient)); \
                            	double ucbCapacity = 0.0, ucbOther = 0.0, ucbGiveUp = 0.0; \
                            	if (likely(random_generate(randomFallback) % 100 < 95)) { \
                                    double avgCapacityCycles = ((double)current_memoized->cyclesCapacity) / current_memoized->believedCapacity; \
                                    double avgTransientCycles = ((double)current_memoized->cyclesTransient) / current_memoized->believedTransient; \
                                    double avgGiveUpCycles = ((double)current_memoized->cyclesGiveUp) / current_memoized->believedGiveUp; \
                            	    ucbCapacity = (100.0 / avgCapacityCycles) + sqrt(logSum / ((double)current_memoized->believedCapacity)); \
                            	    ucbOther = (100.0 / avgTransientCycles) + sqrt(logSum / ((double)current_memoized->believedTransient)); \
                            	    ucbGiveUp = (100.0 / avgGiveUpCycles) + sqrt(logSum / ((double)current_memoized->believedGiveUp)); \
                            	} else { \
                            	    ucbCapacity = rewardCapacity; \
                            	    ucbOther = rewardOther; \
                            	    ucbGiveUp = rewardGiveUp; \
                            	} \
                            	if (ucbOther > ucbCapacity && ucbOther > ucbGiveUp) { \
                            	       current_memoized->believedTransient++; \
                            	       current_memoized->havingCapacityAborts = STUBBORN; \
                                } \
                                else if (ucbGiveUp > ucbOther && ucbGiveUp > ucbCapacity ) { \
                                    current_memoized->believedGiveUp++; \
                                    current_memoized->havingCapacityAborts = GIVEUP; \
                                } else { \
                                    current_memoized->believedCapacity++; \
                                    current_memoized->havingCapacityAborts = HALVEN; \
                                } \
                            	if (likely(current_memoized->lastCycles != 0)) { \
                            		double changeForWorse = (((double) finalTicks) / ((double) current_memoized->lastCycles)); \
									double changeForBetter = (((double) current_memoized->lastCycles) / ((double) finalTicks)); \
									unsigned short lastRetries = current_memoized->lastRetries; \
									unsigned short currentRetries = current_memoized->retries; \
									if (changeForWorse > 1.40) { \
										current_memoized->retries = current_memoized->bestEverRetries; \
										current_memoized->lastCycles = current_memoized->bestEverCycles; \
										current_memoized->lastRetries = current_memoized->bestEverRetries; \
									} else { \
										if (changeForBetter > 1.05) { \
											current_memoized->retries = currentRetries + (currentRetries - lastRetries); \
										} else if (changeForWorse > 1.05) { \
											current_memoized->retries = currentRetries - (currentRetries - lastRetries); \
										} \
										if (currentRetries > 16) { \
											current_memoized->retries = 16; \
										} else if (currentRetries < 0){ \
											current_memoized->retries = 0; \
										} \
										current_memoized->lastRetries = currentRetries; \
										current_memoized->lastCycles = finalTicks; \
									} \
									if (finalTicks < current_memoized->bestEverCycles) { \
										current_memoized->bestEverCycles = finalTicks; \
										current_memoized->bestEverRetries = currentRetries; \
									} \
								} else { \
									current_memoized->lastCycles = finalTicks; \
									current_memoized->bestEverCycles = finalTicks; \
									current_memoized->lastRetries = current_memoized->retries; \
									current_memoized->bestEverRetries = current_memoized->retries; \
								} \
							} \
                        };



#    define TM_BEGIN_RO()                 TM_BEGIN(0)
#    define TM_RESTART()                  _xabort(0xab);
#    define TM_EARLY_RELEASE(var)         

#  define TM_SHARED_READ(var)         (var)
#  define TM_SHARED_WRITE(var, val)   ({var = val; var;})
#  define TM_LOCAL_WRITE(var, val)    ({var = val; var;})


#  define TM_SHARED_READ(var)         (var)
#  define TM_SHARED_WRITE(var, val)   ({var = val; var;})

#  define TM_SHARED_READ_I(var)         (var)
#  define TM_SHARED_READ_L(var)         (var)
#  define TM_SHARED_READ_P(var)         (var)
#  define TM_SHARED_READ_F(var)         (var)

#  define TM_SHARED_WRITE_I(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_L(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#  define TM_LOCAL_WRITE_I(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_L(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

#endif
