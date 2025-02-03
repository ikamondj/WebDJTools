#include "pch.h"

#ifndef NOMINMAX
#define NOMINMAX  // Prevent Windows from defining min/max macros
#endif

#include "WebRTCConnection.hpp"
#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/thread.h"
#include <iostream>
#include <string>
#include <functional>

// Ensure macros are undefined if they exist
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

