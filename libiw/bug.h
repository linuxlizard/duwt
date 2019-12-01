/* davep 20191129 ; originally from include/linux/bug.h */
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BUG_H
#define BUG_H

#include <stdlib.h>
#include <stdbool.h>
#include "log.h"

/*
 * Since detected data corruption should stop operation on the affected
 * structures. Return value must be checked and sanely acted on by caller.
 */
#if 0
static inline __must_check bool check_data_corruption(bool v) { return v; }
#define CHECK_DATA_CORRUPTION(condition, fmt, ...)			 \
	check_data_corruption(({					 \
		bool corruption = unlikely(condition);			 \
		if (corruption) {					 \
			if (IS_ENABLED(CONFIG_BUG_ON_DATA_CORRUPTION)) { \
				pr_err(fmt, ##__VA_ARGS__);		 \
				BUG();					 \
			} else						 \
				WARN(1, fmt, ##__VA_ARGS__);		 \
		}							 \
		corruption;						 \
	}))
#endif

bool CHECK_DATA_CORRUPTION(bool condition, const char* fmt, ...);

#endif
