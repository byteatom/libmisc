#ifndef __TLV__
#define __TLV__

#if defined(_MSC_VER)
#pragma warning(disable : 4200)
#endif

typedef struct {
	i32 type:8;
	i32 length:24;
	byte value[0];
}tlv;

#endif
