#include <gtest/gtest.h>

#include <concepts>
#include <memory>
#include <string>

#ifdef USE_STD_MAP
#include <map>
#else
#include "MapTree.h"
#endif

using KEY_T = int;
using VALUE_T = std::string;

class NoCopy
{
  public:
	int m_value;
	NoCopy(int value) noexcept : m_value(value) {}
	NoCopy(NoCopy&&) = default;
	NoCopy(const NoCopy&) { throw std::runtime_error("copy failed (copy ctor)"); }

	NoCopy& operator=(NoCopy&&) = default;
	NoCopy& operator=(const NoCopy&) { throw std::runtime_error("copy failed (copy operator=)"); }
};

#define COND_TEST(TEST_NAME, ...)                                                                                      \
	template< typename T >                                                                                             \
	void check_##TEST_NAME()                                                                                           \
	{                                                                                                                  \
		FAIL();                                                                                                        \
	}                                                                                                                  \
	template< typename T >                                                                                             \
		requires(__VA_ARGS__)                                                                                          \
	void check_##TEST_NAME();                                                                                          \
	TEST(TEST_GROUP_NAME, TEST_NAME)                                                                                   \
	{                                                                                                                  \
		check_##TEST_NAME< MAP_TYPE >();                                                                               \
	}                                                                                                                  \
	template< typename T >                                                                                             \
		requires(__VA_ARGS__)                                                                                          \
	void check_##TEST_NAME()

#define CHECK_REQ(...)                                                                                                 \
	{                                                                                                                  \
		if constexpr (__VA_ARGS__)                                                                                     \
		{                                                                                                              \
			SUCCEED();                                                                                                 \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			FAIL();                                                                                                    \
		}                                                                                                              \
	}
