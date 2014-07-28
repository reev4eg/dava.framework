/*==================================================================================
    Copyright (c) 2008, DAVA, INC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA, INC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVA, INC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#include "Platform/DeviceInfo.h"

#ifdef __DAVAENGINE_MACOS__

#import <Foundation/NSLocale.h>
#import <Foundation/NSTimeZone.h>
#include "Utils/StringFormat.h"
#include "OpenUDIDMacOS.h"

namespace DAVA
{

String DeviceInfo::GetVersion()
{
	return "Not yet implemented";
}

String DeviceInfo::GetManufacturer()
{
	return "Apple inc.";
}

String DeviceInfo::GetModel()
{
	return "Not yet implemented";
}

String DeviceInfo::GetLocale()
{
	NSLocale *english = [[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"] autorelease];

	NSString* langID = [[NSLocale preferredLanguages] objectAtIndex:0];
	NSString *lang = [english displayNameForKey:NSLocaleLanguageCode value:langID];

	String res = Format("%s (%s)", [langID UTF8String], [lang UTF8String]);
	return res;
}

String DeviceInfo::GetRegion()
{
	NSLocale *english = [[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"] autorelease];

	NSString *countryCode = [[NSLocale currentLocale] objectForKey: NSLocaleCountryCode];
	NSString *country = [english displayNameForKey: NSLocaleCountryCode value: countryCode];

	String res = Format("%s (%s)", [countryCode UTF8String], [country UTF8String]);
	return res;
}

String DeviceInfo::GetTimeZone()
{
	NSTimeZone *localTime = [NSTimeZone systemTimeZone];
    
    String res = Format("%s", [[localTime name] UTF8String]);
	return res;
}
    
String DeviceInfo::GetUDID()
{
    OpenUDIDMacOS*  udid = [[[OpenUDIDMacOS alloc] init] autorelease];
    return [[udid value] UTF8String];
}
    
WideString DeviceInfo::GetName()
{
    NSString * deviceName = [[NSHost currentHost] localizedName];
    
    NSStringEncoding pEncode    =   CFStringConvertEncodingToNSStringEncoding ( kCFStringEncodingUTF32LE );
    NSData* pSData              =   [ deviceName dataUsingEncoding : pEncode ];
    
    return WideString ( (wchar_t*) [ pSData bytes ], [ pSData length] / sizeof ( wchar_t ) );
}

// Not impletemted yet
String DeviceInfo::GetHTTPProxyHost()
{
	return String();
}

// Not impletemted yet
String DeviceInfo::GetHTTPNonProxyHosts()
{
	return String();
}

// Not impletemted yet
int DeviceInfo::GetHTTPProxyPort()
{
	return 0;
}

eGPUFamily DeviceInfo::GetGPUFamily()
{
    return GPU_UNKNOWN;
}

}

#endif
