#ifndef __AS_TYPES_H__
#define __AS_TYPES_H__


#include <inttypes.h>
#include <limits.h>

typedef uint16_t Channels;
typedef int16_t Channel;

#define NO_CHANNEL -1

typedef int16_t Sample;
#define SAMPLE_MIN INT16_MIN
#define SAMPLE_MAX INT16_MAX

typedef int SampleOffset;		/* Offset in samples. */
typedef unsigned int SampleLength;	/* Length in samples. */

typedef unsigned int FrameLength;	/* Length in frames. */
#define FRAME_LENGTH_MAX (UINT_MAX-1)
#define FRAME_LENGTH_INVALID UINT_MAX

typedef int FrameOffset;		/* Offset in frames. */
#define FRAME_OFFSET_MIN (INT_MIN+1)
#define FRAME_OFFSET_MAX INT_MAX
#define FRAME_OFFSET_INVALID INT_MIN

typedef unsigned int ByteLength;
typedef int ByteOffset;
typedef unsigned int NBlocks;
typedef uint8_t Byte;
#endif /* __AS_TYPES_H__ */
