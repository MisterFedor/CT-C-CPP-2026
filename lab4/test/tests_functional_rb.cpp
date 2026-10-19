#include "helpers.hpp"

#define TEST_GROUP_NAME MapTest_RB

namespace tests_functional_rb
{

#ifdef USE_STD_MAP
	using MAP_TYPE = std::map< KEY_T, VALUE_T >;
	using MAP_CG_TYPE = std::map< KEY_T, VALUE_T, std::greater< KEY_T > >;
	using MAP_U_TYPE = std::map< KEY_T, std::unique_ptr< VALUE_T > >;
	using MAP_TOC_TYPE = std::map< KEY_T, NoCopy >;
#else
	using MAP_TYPE = MapTree< KEY_T, VALUE_T, RBTree >;
	using MAP_CG_TYPE = MapTree< KEY_T, VALUE_T, RBTree, std::greater< KEY_T > >;
	using MAP_U_TYPE = MapTree< KEY_T, std::unique_ptr< VALUE_T >, RBTree >;
	using MAP_TOC_TYPE = MapTree< KEY_T, NoCopy, RBTree >;
#endif

#include "tests_functional.inc"
}	 // namespace tests_functional_rb

#undef TEST_GROUP_NAME
