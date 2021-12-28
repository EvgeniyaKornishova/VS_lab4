#include "lock.h"

#define LOCK_PASS_DEFAULT_LEN 9
const uint8_t LOCK_PASS_DEFAULT_VALUE[] = {1,2,3,4,5,6,7,8,9};
const uint32_t LOCK_PASS_INPUT_TIME_LIMIT = MIN_SEC_TO_MS(0, 30);

void lock_init(Lock *lock, uint8_t * pass_len, uint8_t * pass_value)
{
  if (pass_len == NULL){
	  memcpy(lock->password, LOCK_PASS_DEFAULT_VALUE, LOCK_PASS_DEFAULT_LEN);
  	  lock->length = LOCK_PASS_DEFAULT_LEN;
  } else {
	  memcpy(lock->password, pass_value, *pass_len);
	  lock->length = *pass_len;
  }
  lock->number_of_mistakes = 0;
  lock->input_time_start = 0;
}

void lock_start_timer(Lock *lock)
{
	lock->input_time_start = HAL_GetTick();
}

void lock_stop_timer(Lock *lock)
{
	lock->input_time_start = 0;
}

uint8_t lock_is_input_time_expired(Lock *lock)
{
  if (lock->input_time_start == 0)
	  return 0;

  return HAL_GetTick() > (lock->input_time_start + LOCK_PASS_INPUT_TIME_LIMIT);
}

void lock_reset_number_of_mistakes(Lock *lock)
{
  lock->number_of_mistakes = 0;
}

uint8_t lock_is_blocked(Lock *lock){
	return lock->number_of_mistakes >= LOCK_PASS_MAX_TRY;
}

uint8_t lock_unlock(Lock* lock, uint8_t pass_len, uint8_t * pass_value){
	if (lock->length != pass_len){
		lock->number_of_mistakes++;
		return 0;
	}

	for (int i = 0; i < pass_len; i++){
		if (lock->password[i] != pass_value[i]){
			lock->number_of_mistakes++;
			return 0;
		}
	}

	lock_reset_number_of_mistakes(lock);
	return 1;
}
