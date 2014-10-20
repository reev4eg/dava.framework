#include "IdeUnitTestsSupport.h"

IDE_TEST_CASE(MinimalTestCase, "test")
{
    int i = 1;
    IDE_REQUIRE(1 == i);
}
IDE_TEST_CASE_END