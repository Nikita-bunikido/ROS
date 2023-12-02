#ifndef _ROS_FOR_MODULES_ALL
#define _ROS_FOR_MODULES_ALL

#ifdef _ROS_HEADER
#   undef _ROS_HEADER
#endif /* _ROS_HEADER */

#ifdef _INCLUDE_FONT
#   undef _INCLUDE_FONT
#endif /* _INCLUDE_FONT */

/* system modules */
#include <ros/builtin.h>
#include <ros/chip8.h>
#include <ros/font.h>
#include <ros/log.h>
#include <ros/ros-internal.h>
#include <ros/video.h>

/* drivers */
#include <drivers/keyboard.h>
#include <drivers/memory.h>
#include <drivers/spi.h>
#include <drivers/st7735.h>

#endif /* _ROS_FOR_MODULES_ALL */