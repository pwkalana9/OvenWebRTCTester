#pragma once

#include <stdio.h>
#include <iostream>
#include <stdarg.h>
#include <errno.h>

#ifndef MAX_PATH
	#define MAX_PATH (256)
#endif 

extern int errno;
#include "linux_definition.h"

//====================================================================================================
// Config 관련 정의
//====================================================================================================
#include "config_define.h"
#include "./common/config_parser.h"
#include "./common/common_function.h" 
