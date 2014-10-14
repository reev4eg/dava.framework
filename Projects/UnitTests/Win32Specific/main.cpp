/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "DAVAEngine.h"

#ifdef VS2012_UNIT_TESTS

#include "CppUnitTest.h"

#include "../Classes/HashMapTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

typedef Microsoft::VisualStudio::CppUnitTestFramework::Logger Log;

namespace UnitTestsVS
{		

TEST_MODULE_INITIALIZE(ModuleInitialize)
{
	Log::WriteMessage("In Module Initialize\n");
}

TEST_CLASS(HashMapVSTest)
{
public:

	HashMapVSTest()
	{
		Log::WriteMessage("In HashMapVSTest constructor\n");
	}
	~HashMapVSTest()
	{
		Log::WriteMessage("In ~HashMapVSTest destructor\n");
	}

	TEST_CLASS_INITIALIZE(HashMapVSTestInit)
	{
		Log::WriteMessage("In HashMapVSTest Initialize class\n");
	}
	TEST_CLASS_CLEANUP(HashMapVSTestClear)
	{
		Log::WriteMessage("In HashMapVSTest Cleanup class\n");
	}

	
	BEGIN_TEST_METHOD_ATTRIBUTE(Method2)
        TEST_OWNER(L"OwnerName")
        TEST_PRIORITY(1)
	END_TEST_METHOD_ATTRIBUTE()

	TEST_METHOD_INITIALIZE(InitBeforeEveryTestMethod)
	{
		Log::WriteMessage("init before every method\n");
	}

	TEST_METHOD(Method1)
	{   
		Log::WriteMessage("In Method1\n");
		Assert::AreEqual(0, 1);
	}

	TEST_METHOD(Method2)
	{
		Log::WriteMessage("In Method2\n");
		Assert::Fail(L"Fail");
	}

	TEST_METHOD_CLEANUP(CleanupBeforeEveryTestMethod)
	{
		Log::WriteMessage("clear after every method\n");
	}
}; // end test_class

TEST_MODULE_CLEANUP(ModuleCleanup)
{
	Logger::WriteMessage("In Module Cleanup\n");
}

} // end UnitTestsVS namespace

#else

int APIENTRY WinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPSTR    lpCmdLine,
					   int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	return DAVA::Core::Run(0, 0, hInstance);
}

#endif // VS2012_UNIT_TESTS


/*int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int       nCmdShow)
{
	// find leaks mega-string: {,,msvcr71d.dll}_crtBreakAlloc
	//
	// POSSIBLE LEAKS LIST:
	// remember -- always clear all allocated data for static STL containers
	// 
	return 0;
}*/