#ifndef __TIMEPOSITION_H__
#define __TIMEPOSITION_H__
#include <limits.h>
typedef int TimePosition;	/* Time code expressed as 1/600 s
				   600 was chosen because it can be divided
				   by 50,60 and 24.
				*/
#define TIME_POSITION_FRACTIONS 600
#define TIME_POSITION_MIN INT_MIN
#define TIME_POSITION_MAX INT_MAX

typedef unsigned int TimeDuration;
				   
#endif /* __TIMEPOSITION_H__ */
