// The single translation unit containing the webview implementation (only built with HATENIUM_ENABLE_YOUTUBE).
#ifdef HATENIUM_YOUTUBE
#define WEBVIEW_STATIC
#include "webview.h"
#endif
