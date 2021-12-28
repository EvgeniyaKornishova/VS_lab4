#include "main.h"
#include <string.h>

#ifndef lock_h
#define lock_h

#define LOCK_PASS_LEN_MIN 8
#define LOCK_PASS_LEN_MAX 12

#define LOCK_PASS_MAX_TRY 3

#define MIN_SEC_TO_MS(minutes, seconds) ((minutes) * 60 * 1000 + (seconds) * 1000)



typedef struct
{
  uint8_t length;
  uint8_t password[12];
  uint8_t number_of_mistakes;
  uint32_t input_time_start;
} Lock;


void lock_init(Lock *lock, uint8_t * pass_len, uint8_t * pass_value);
uint8_t lock_is_input_time_expired(Lock *lock);
void lock_reset_number_of_mistakes(Lock *lock);
uint8_t lock_is_blocked(Lock *lock);
uint8_t lock_unlock(Lock* lock, uint8_t pass_len, uint8_t * pass_value);
void lock_start_timer(Lock *lock);
void lock_stop_timer(Lock *lock);

#endif

