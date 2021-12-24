/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 * @author			: 0x6a616e65
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "fonts.h"
#include "lock.h"
#include "kb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	APP_MODE_PASS_INPUT = 0, APP_MODE_PASS_CHANGE = 1
} APP_MODE;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MSG_PASS_CHANGED 	"CHANGED"
#define MSG_PASS_UNCHANGED 	"UNCHANGED"
#define MSG_PASS_LEN_ERROR 	"7 < L < 13"

#define MSG_APP_MODE_PASS_INPUT 	"INPUT"
#define MSG_APP_MODE_PASS_CHANGE 	"CHANGE"

#define MSG_PASS_CORRECT 	"CORRECT"
#define MSG_PASS_WRONG		"WRONG"
#define MSG_PASS_MAX_TRY  	"ALERT"

#define KEY_BUFFER_LEN 8

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MIN_SEC_TO_MS(minutes, seconds) ((minutes) * 60 * 1000 + (seconds) * 1000)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

APP_MODE app_mode = APP_MODE_PASS_INPUT;

uint8_t keys_buffer[KEY_BUFFER_LEN];
uint8_t *keys_buffer_read_p;
uint8_t *keys_buffer_write_p;

uint8_t input[LOCK_PASS_LEN_MAX] = { 0 };
uint8_t input_len = 0;

uint8_t screen_needs_to_update = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void process_key(uint8_t key, Lock *lock);
uint8_t read_key_from_keys_buffer();
void write_key_to_keys_buffer(uint8_t key);

void switch_app_mode();

void print_message(char *str);
void draw_display(uint8_t pass_len);
void print_pass(uint8_t len);

void pass_input_confirm(Lock *lock);
void pass_change_confirm(Lock *lock);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void print_message(char *str) {
	oled_Fill(White);

	oled_SetCursor(9, 23);
	oled_WriteString(str, Font_11x18, Black);

	screen_needs_to_update = 1;
	HAL_Delay(MIN_SEC_TO_MS(0, 2));
}

void draw_display(uint8_t pass_len) {
	oled_Fill(White);

	oled_DrawHLine(0, 127, 10, Black); // Header outline
	oled_DrawHLine(0, 127, 53, Black); // Footer outline

	oled_DrawSquare(21, 106, 26, 37, Black); // Password field border

	// Menu
	oled_SetCursor(9, 54);
	oled_WriteString("CO-10 CL-11 MD-12", Font_7x10, Black);

	// Header - current mode
	oled_SetCursor(72, 0);
	if (app_mode == APP_MODE_PASS_INPUT)
		oled_WriteString(MSG_APP_MODE_PASS_INPUT, Font_7x10, Black);
	else
		oled_WriteString(MSG_APP_MODE_PASS_CHANGE, Font_7x10, Black);

	print_pass(pass_len);

	screen_needs_to_update = 1;
}

void print_pass(uint8_t len) {
	char pass[LOCK_PASS_LEN_MAX + 1] = { '\0' };
	int i = 0;
	for (; i < len; i++)
		pass[i] = '*';
	for (; i < LOCK_PASS_LEN_MAX; i++)
		pass[i] = ' ';

	oled_SetCursor(22, 27);
	oled_WriteString(pass, Font_7x10, Black);
}

void switch_app_mode() {
	if (app_mode == APP_MODE_PASS_INPUT)
		app_mode = APP_MODE_PASS_CHANGE;
	else
		app_mode = APP_MODE_PASS_INPUT;

	draw_display(0);
}

void pass_input_confirm(Lock *lock) {
	if (lock_unlock(lock, input_len, input))
		print_message(MSG_PASS_CORRECT);
	else {
		print_message(MSG_PASS_WRONG);

		if (lock_is_blocked(lock)) {
			lock_reset_number_of_mistakes(lock);
			print_message(MSG_PASS_MAX_TRY);
		}
	}
	draw_display(0);
}

void pass_change_confirm(Lock *lock) {
	if (input_len < LOCK_PASS_LEN_MIN) {
		print_message(MSG_PASS_LEN_ERROR);
		draw_display(0);
	} else {
		lock_init(lock, &input_len, input);
		print_message(MSG_PASS_CHANGED);
		switch_app_mode();
	}
}

void process_key(uint8_t key, Lock *lock) {
	switch (key) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		if (input_len < LOCK_PASS_LEN_MAX) {
			input[input_len++] = key;

			draw_display(input_len);
		}
		break;
	case 10: // Confirm
		if (app_mode == APP_MODE_PASS_INPUT)
			pass_input_confirm(lock);
		else
			// APP_MODE_PASS_CHANGE
			pass_change_confirm(lock);

		input_len = 0;

		break;
	case 11:	// Reset
		input_len = 0;
		draw_display(0);
		break;
	case 12:	// Change mode
		if (app_mode == APP_MODE_PASS_CHANGE)
			print_message(MSG_PASS_UNCHANGED);
		input_len = 0;
		switch_app_mode();
		break;
	}
}

// TODO: verify this function
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		if (screen_needs_to_update && !kb_state) {
			oled_UpdateScreen();
			screen_needs_to_update = 0;
		}

		uint8_t key = 0;

		static uint8_t prev_confirmed_key = 0;
		static uint8_t prev_key = 0;
		uint8_t current_key = kb_read();

		// key is really pressed or it is just an contact bounce?
		if (current_key == prev_key) {
			if (current_key != prev_confirmed_key) {
				prev_confirmed_key = current_key;
				key = current_key;
			}
		} else
			prev_key = current_key;

		// if key isn't pressed
		if (!key)
			return;

		write_key_to_keys_buffer(key);
		return;
	}
}

uint8_t read_key_from_keys_buffer() {
	// critical section
	uint32_t primask_bit = __get_PRIMASK(); /**< backup PRIMASK bit */
	__disable_irq(); /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

	uint8_t key = *(keys_buffer_read_p++);
	if (keys_buffer_read_p >= keys_buffer + KEY_BUFFER_LEN)
		keys_buffer_read_p = keys_buffer;

	__set_PRIMASK(primask_bit); /**< Restore PRIMASK bit*/

	return key;
}

void write_key_to_keys_buffer(uint8_t key) {
	// critical section
	uint32_t primask_bit = __get_PRIMASK(); /**< backup PRIMASK bit */
	__disable_irq(); /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

	*(keys_buffer_write_p++) = key;

	if (keys_buffer_write_p >= keys_buffer + KEY_BUFFER_LEN)
		keys_buffer_write_p = keys_buffer;

	__set_PRIMASK(primask_bit); /**< Restore PRIMASK bit*/
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == &hi2c1 && kb_state)
		kb_continue();
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == &hi2c1 && kb_state)
		kb_continue();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

	oled_Init();

	Lock lock = { };
	lock_init(&lock, NULL, NULL);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	keys_buffer_read_p = keys_buffer;
	keys_buffer_write_p = keys_buffer;
	HAL_TIM_Base_Start_IT(&htim6);
	uint8_t key;
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if (app_mode == APP_MODE_PASS_INPUT)
			if (lock_is_input_time_expired(&lock)) {
				input_len = 0;
				draw_display(0);
			}

		key = 0;

		if (keys_buffer_read_p != keys_buffer_write_p)
			key = read_key_from_keys_buffer();

		if (key != 0)
			process_key(key, &lock);
		else
			HAL_Delay(100);

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
