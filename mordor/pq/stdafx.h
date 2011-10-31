// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifdef MSVC
#pragma once

#include <boost/bind.hpp>
#include "mordor/assert.h"
#include "mordor/config.h"
#include "mordor/iomanager.h"
#include "mordor/log.h"
#include "mordor/util.h"
#include "mordor/streams/buffer.h"
#include "mordor/streams/stream.h"
#include "mordor/endian.h"
#include "mordor/socket.h"
#include "targetver.h"
#include "connection.h"
#include "connectionpool.h"
#include "exception.h"
#include "preparedstatement.h"
#include "result.h"
#include "targetver.h"
#include "transaction.h"


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif


// TODO: reference additional headers your program requires here
