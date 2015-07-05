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
extern void test_RegularPositive();
extern void test_PositiveOverflowBoundary();
extern void test_PositiveOverflowBig();
extern void test_Negative();
extern void test_IllegalChar();


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
  Unity.TestFile = "atouint.c";
  UnityBegin("atouint.c");
  RUN_TEST(test_RegularPositive, 9);
  RUN_TEST(test_PositiveOverflowBoundary, 17);
  RUN_TEST(test_PositiveOverflowBig, 24);
  RUN_TEST(test_Negative, 31);
  RUN_TEST(test_IllegalChar, 38);

  return (UnityEnd());
}
