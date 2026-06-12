// 唯一一個定義 doctest main 的 translation unit。其餘 *_test.cpp 只 include
// <doctest/doctest.h> 寫 TEST_CASE，連結到一起就組成單一的 ds_tests 執行檔。
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
