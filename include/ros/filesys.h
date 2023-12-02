#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include <inttypes.h>

#include <ros/ros-for-headers.h>

#define PAGE_SIZE   128
#define PAGE_REST_SIZE      ((PAGE_SIZE) - (sizeof(struct Page_Header)))

typedef uint16_t mpointer;
typedef uint8_t mbool;

typedef uint8_t mflags;
enum MFLAGS {
    /* General */
    MFLAG_READ = ( 1 << 0 ),
    MFLAG_WRITE = ( 1 << 1 ),
    MFLAG_HIDDEN = ( 1 << 2 ),
    MFLAG_SYSTEM = ( 1 << 3 ),
    MFLAG_TEMP = ( 1 << 4 ),

    /* Entry types */
    MFLAG_DIRECTORY = ( 1 << 5 ),
    MFLAG_EXECUTABLE = ( 1 << 6 )
};

typedef uint8_t Page_Type;
enum PAGE_TYPE {
    PAGE_TYPE_RAW = 0,
    PAGE_TYPE_LIST,
    PAGE_TYPE_DESCRIPTOR
};

struct PACKED Page_Header {
    mbool in_use;
    mpointer next;
    Page_Type type;
};

struct PACKED Page_Descriptor {
    mflags flags;
    time_t time_stamp;
    uint8_t padding[PAGE_REST_SIZE - sizeof(mflags) - sizeof(time_stamp)];
};

struct PACKED Page_List {
    mpointer entrys[PAGE_REST_SIZE / sizeof(mpointer)];
};

struct PACKED Page {
    struct Page_Header header;
    
    union {
        uint8_t raw[PAGE_SIZE - sizeof(struct Page_Header)];
        struct Page_Descriptor descriptor;
        struct Page_List list;
    };
};

#endif /* _FILE_SYS_H */