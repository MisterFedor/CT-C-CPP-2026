#include "helpers.hpp"

#define TEST_GROUP_NAME MapNamedRequirements_RB

namespace tests_reqs_rb
{
#ifdef USE_STD_MAP
	using MAP_TYPE = std::map< KEY_T, VALUE_T >;
#else
	using MAP_TYPE = MapTree< KEY_T, VALUE_T, RBTree >;
#endif

#include "tests_reqs.inc"
}	 // namespace tests_reqs_rb

#undef TEST_GROUP_NAME
