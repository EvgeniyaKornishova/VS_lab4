/*
 * utils.h
 *
 *  Created on: Nov 22, 2021
 *      Author: jenni
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include <stdio.h>

#define CHK_HAL(action) \
	do { \
		HAL_StatusTypeDef retcode = action; \
		if (retcode != HAL_OK) { \
			return retcode; \
		} \
	} while (0);

#endif /* INC_UTILS_H_ */
