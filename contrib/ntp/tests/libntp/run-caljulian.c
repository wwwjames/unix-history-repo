/* AUTOGENERATED FILE. DO NOT EDIT. */

//=======Test Runner Used To Run Each Test Below=====
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

//=======Automagically Detected Files To Include=====
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
void resetTest(void);
extern void test_RegularTime();
extern void test_LeapYear();
extern void test_uLongBoundary();
extern void test_uLongWrapped();


//=======Test Reset Option=====
void resetTest()
{
  tearDown();
  setUp();
}

char *progname;


//=======MAIN=====
int main(int argc, char *argv[])
{
  progname = argv[0];
  Unity.TestFile = "caljulian.c";
  UnityBegin("caljulian.c");
  RUN_TEST(test_RegularTime, 74);
  RUN_TEST(test_LeapYear, 85);
  RUN_TEST(test_uLongBoundary, 96);
  RUN_TEST(test_uLongWrapped, 107);

  return (UnityEnd());
}
