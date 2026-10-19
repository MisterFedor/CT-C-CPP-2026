
#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>

static void printresults(const testing::UnitTest* tests)
{
	std::ofstream f{ "results.txt" };

	if (!f.is_open())
	{
		throw std::logic_error("File 'results.txt' can't be opened!");
	}

	const int passed = tests->successful_test_count();
	const int total = tests->total_test_count();

	f << passed << ' ' << total << '\n';
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	const int exitcode = RUN_ALL_TESTS();

	const testing::UnitTest* tests = testing::UnitTest::GetInstance();
	printresults(tests);

	return exitcode;
}
