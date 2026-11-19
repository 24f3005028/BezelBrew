#pragma once
// A Desktop OAuth application is configured by the publisher once per build.
// Desktop client identifiers are public; user tokens never belong in a build.
namespace PublisherGoogle {
#ifdef BEZELBREW_PUBLISHER_GOOGLE
#include <bezelbrew_google_generated.hpp>
#else
static const wchar_t clientId[]=L"";
static const char clientSecret[]="";
#endif
static bool configured(){if(!clientId[0])return false;return clientSecret[0]!=0;}
}
