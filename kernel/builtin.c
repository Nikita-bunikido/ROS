#include <avr/pgmspace.h>

#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stddef.h>

#include <ros/video.h>
#include <ros/builtin.h>
#include <ros/ros-for-modules.h>

#define DEFINE_SYSTEM_REX(_name, ...)		static const uint8_t _name##_rex[] PROGMEM = { __VA_ARGS__ };

DEFINE_SYSTEM_REX(cls, 
	0x00, 0xe0, 0x81, 0x13, 0x04, 0x02  
)

int get_builtin_program(struct Builtin_Program *info, const char *name) {   
	static const struct Builtin_Program programs[] = {
		{ "cls",  (void *)cls_rex, ARR_SIZE(cls_rex) },
	};

	if (!info) return -1;
	strncpy(info->name, name, sizeof info->name);

	#if defined(__GNUC__)
	#	pragma GCC unroll 2
	#endif /* __GNUC__ */
	for (unsigned i = 0; i < ARR_SIZE(programs); i ++) {
        if (caseless_cmp(programs[i].name, name))
            continue;
    
		memcpy(info, programs + i, sizeof(*info));
		return 0;
	}

	return -1;
}