// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _ALLOW_RTCc_IN_STL

#ifdef _DEBUG
#define LOGGER
#endif

// Windows Header Files
#include <windows.h>
#include <mmsystem.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "tools.h"
#include "calc.h"
#include "config.h"

#include "value.h"
#include "operators.h"
#include "etree.h"

#include "view.h"
#include "main_view.h"
#include "view_input.h"

// reference additional headers your program requires here
