#if defined(__DAVAENGINE_WIN32__) || defined(_WIN32)

// this file from https://github.com/colonelsammy/Catch/tree/master/single_include_projects
#include "catch.hpp"

#define IDE_TEST_CASE_START( name, ... ) TEST_CASE( #name, __VA_ARGS__ )
#define IDE_REQUIRE( expr ) REQUIRE( expr )
#define IDE_SECTION( expr ) SECTION( expr )
#define IDE_TEST_CASE_END // on OS X inside Objective-C need to generate @end on Win32 empty

#else

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#define TEST_OBJECTIVEC_METHOD_NAME_GEN( name ) test##name

#define IDE_TEST_CASE(name, tags) \
	@interface name : XCTestCase \
	@end \
	@implementation name \
	-(void) TEST_OBJECTIVEC_METHOD_NAME_GEN( name )

#define IDE_REQUIRE( expr ) XCTAssert( (expr), @Pass);
#define IDE_SECTION( expr ) // Do I need gen some try catch here?
#define IDE_TEST_CASE_END @end

#endif