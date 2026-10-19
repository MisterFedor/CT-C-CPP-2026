COND_TEST(DefaultConstructor, std::is_default_constructible_v< MAP_TYPE >)
{
	MAP_TYPE map;
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);
	EXPECT_EQ(map.begin(), map.end());
}

COND_TEST(InitializerListConstructor, std::is_constructible_v< MAP_TYPE, std::initializer_list< typename MAP_TYPE::value_type > >)
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	EXPECT_EQ(map.size(), 3u);
	EXPECT_EQ(map.at(1), "one");
	EXPECT_EQ(map.at(2), "two");
	EXPECT_EQ(map.at(3), "three");
}

COND_TEST(CopyConstructor, std::is_copy_constructible_v< MAP_TYPE >)
{
	MAP_TYPE original;
	original[10] = "ten";
	original[20] = "twenty";

	MAP_TYPE copied(original);

	EXPECT_EQ(copied.size(), original.size());
	EXPECT_EQ(copied[10], "ten");
	EXPECT_EQ(copied[20], "twenty");

	copied[30] = "thirty";
	EXPECT_EQ(original.count(30), 0u);
	EXPECT_EQ(copied.count(30), 1u);
}

COND_TEST(CopyAssignment, std::is_copy_assignable_v< MAP_TYPE >)
{
	MAP_TYPE map1{ { 1, "one" } };
	MAP_TYPE map2{ { 2, "two" }, { 3, "three" } };

	map2 = map1;

	EXPECT_EQ(map2.size(), 1u);
	EXPECT_EQ(map2.at(1), "one");
	EXPECT_EQ(map2.count(2), 0u);
}

COND_TEST(MoveConstructor, std::is_move_constructible_v< MAP_TYPE >)
{
	MAP_TYPE original;
	original[100] = "hundred";

	MAP_TYPE moved(std::move(original));

	EXPECT_EQ(moved.at(100), "hundred");
	EXPECT_TRUE(original.empty());
}

COND_TEST(MoveAssignment, std::is_move_assignable_v< MAP_TYPE >)
{
	MAP_TYPE map1{ { 1, "one" }, { 2, "two" } };
	MAP_TYPE map2;

	map2 = std::move(map1);

	EXPECT_EQ(map2.size(), 2u);
	EXPECT_EQ(map2.at(1), "one");
	EXPECT_EQ(map2.at(2), "two");
	EXPECT_TRUE(map1.empty());
}

COND_TEST(Insert_NewElement, requires(MAP_TYPE m) { m.insert(std::declval< typename MAP_TYPE::value_type >()); })
{
	MAP_TYPE map;

	auto [it, inserted] = map.insert({ 42, "answer" });

	EXPECT_TRUE(inserted);
	EXPECT_EQ(it->first, 42);
	EXPECT_EQ(it->second, "answer");
	EXPECT_EQ(map.size(), 1u);
}

COND_TEST(Insert_DuplicateKey, requires(MAP_TYPE m) { m.insert(std::declval< typename MAP_TYPE::value_type >()); })
{
	MAP_TYPE map;
	map.insert({ 1, "first" });

	auto [it, inserted] = map.insert({ 1, "second" });

	EXPECT_FALSE(inserted);
	EXPECT_EQ(it->second, "first");
	EXPECT_EQ(map.size(), 1u);
}

COND_TEST(Insert_MoveSemantics, requires(MAP_U_TYPE m) { m.insert(std::declval< typename MAP_U_TYPE::value_type >()); })
{
	MAP_U_TYPE map;
	auto ptr = std::make_unique< VALUE_T >("moved");
	VALUE_T* raw_ptr = ptr.get();

	auto [it, inserted] = map.insert({ 1, std::move(ptr) });

	EXPECT_TRUE(inserted);
	EXPECT_EQ(*it->second, "moved");
	EXPECT_EQ(ptr, nullptr);
}

COND_TEST(Emplace_NewElement, requires(MAP_TYPE m) { m.emplace(7, "seven"); })
{
	MAP_TYPE map;

	auto [it, inserted] = map.emplace(7, "seven");

	EXPECT_TRUE(inserted);
	EXPECT_EQ(it->first, 7);
	EXPECT_EQ(it->second, "seven");
}

COND_TEST(Emplace_DuplicateKey, requires(MAP_TYPE m) { m.emplace(7, "seven"); })
{
	MAP_TYPE map;
	map.emplace(1, "one");

	auto [it, inserted] = map.emplace(1, "uno");

	EXPECT_FALSE(inserted);
	EXPECT_EQ(it->second, "one");
}

COND_TEST(OperatorSubscript_InsertAndUpdate, requires(MAP_TYPE m) { m[10] = "ten"; })
{
	MAP_TYPE map;

	map[10] = "ten";
	EXPECT_EQ(map.at(10), "ten");
	EXPECT_EQ(map.size(), 1u);

	map[10] = "updated";
	EXPECT_EQ(map.at(10), "updated");
	EXPECT_EQ(map.size(), 1u);
}

COND_TEST(OperatorSubscript_DefaultConstructsValue, requires(MAP_TYPE m) { m[999]; })
{
	MAP_TYPE map;

	const VALUE_T& ref = map[999];

	EXPECT_TRUE(ref.empty());
	EXPECT_EQ(map.count(999), 1u);
}

COND_TEST(Find_ExistingKey, requires(MAP_TYPE m) { m.find(2); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto it = map.find(2);

	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->first, 2);
	EXPECT_EQ(it->second, "two");
}

COND_TEST(Find_NonExistingKey, requires(MAP_TYPE m) { m.find(999); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" } };

	auto it = map.find(999);

	EXPECT_EQ(it, map.end());
}

COND_TEST(Count_ExistingKey, requires(const MAP_TYPE m) { m.count(5); })
{
	MAP_TYPE map = { { 5, "five" } };

	EXPECT_EQ(map.count(5), 1u);
}

COND_TEST(Count_NonExistingKey, requires(const MAP_TYPE m) { m.count(5); })
{
	MAP_TYPE map = { { 5, "five" } };

	EXPECT_EQ(map.count(999), 0u);
}

COND_TEST(At_ExistingKey, requires(MAP_TYPE m) { m.at(42); })
{
	MAP_TYPE map = { { 42, "answer" } };

	EXPECT_EQ(map.at(42), "answer");
}

COND_TEST(At_NonExistingKey_Throws, requires(const MAP_TYPE m) { m.at(999); })
{
	MAP_TYPE map = { { 1, "one" } };

	EXPECT_THROW({ [[maybe_unused]] auto& val = map.at(999); }, std::out_of_range);

	EXPECT_EQ(map.at(1), "one");
}

COND_TEST(At_ConstMap, requires(const MAP_TYPE m) { m.at(1); })
{
	const MAP_TYPE map = { { 1, "one" } };

	EXPECT_EQ(map.at(1), "one");
}

COND_TEST(Erase_ByIterator, requires(MAP_TYPE m) { m.erase(m.find(2)); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto it = map.find(2);
	ASSERT_NE(it, map.end());

	auto next_it = map.erase(it);

	EXPECT_EQ(map.count(2), 0u);
	EXPECT_EQ(map.size(), 2u);
	EXPECT_EQ(next_it->first, 3);
}

COND_TEST(Erase_ByKey_Existing, requires(MAP_TYPE m) { m.erase(2); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto count = map.erase(2);

	EXPECT_EQ(count, 1u);
	EXPECT_EQ(map.count(2), 0u);
	EXPECT_EQ(map.size(), 2u);
}

COND_TEST(Erase_ByKey_NonExisting, requires(MAP_TYPE m) { m.erase(2); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" } };

	auto count = map.erase(999);

	EXPECT_EQ(count, 0u);
	EXPECT_EQ(map.size(), 2u);
}

COND_TEST(Clear, requires(MAP_TYPE m) { m.clear(); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	map.clear();

	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);
	EXPECT_EQ(map.begin(), map.end());
}

COND_TEST(
	BeginEnd_EmptyMap,
	requires(MAP_TYPE m) {
		m.begin();
		m.end();
	})
{
	MAP_TYPE map;

	EXPECT_EQ(map.begin(), map.end());
}

COND_TEST(
	Iterator_Ordering_Ascending,
	requires(MAP_TYPE m) {
		m.begin();
		m.end();
	})
{
	MAP_TYPE map;
	map[5] = "five";
	map[1] = "one";
	map[3] = "three";
	map[2] = "two";

	std::vector< KEY_T > keys;
	for (const auto& [k, v] : map)
	{
		keys.push_back(k);
	}

	EXPECT_EQ(keys, (std::vector< KEY_T >{ 1, 2, 3, 5 }));
}

COND_TEST(
	ConstIterator,
	requires(const MAP_TYPE m) {
		m.cbegin();
		m.cend();
	})
{
	const MAP_TYPE map = { { 1, "one" }, { 2, "two" } };

	std::vector< KEY_T > keys;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		keys.push_back(it->first);
	}

	EXPECT_EQ(keys, (std::vector< KEY_T >{ 1, 2 }));
}

COND_TEST(
	Iterator_IncrementAndDecrement,
	requires(typename MAP_TYPE::iterator it) {
		++it;
		it++;
		--it;
		it--;
	})
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto it = map.begin();
	EXPECT_EQ(it->first, 1);

	++it;
	EXPECT_EQ(it->first, 2);

	it++;
	EXPECT_EQ(it->first, 3);

	--it;
	EXPECT_EQ(it->first, 2);

	it--;
	EXPECT_EQ(it->first, 1);
}

COND_TEST(
	Iterator_DereferenceAndModification,
	requires(typename MAP_TYPE::iterator it) {
		it->first;
		(*it).second;
		it->second = "modified";
	})
{
	MAP_TYPE map;
	map.insert({ 42, "answer" });

	auto it = map.begin();

	EXPECT_EQ(it->first, 42);
	EXPECT_EQ((*it).second, "answer");

	it->second = "modified";
	EXPECT_EQ(map.at(42), "modified");
}

COND_TEST(LowerBound_ExactMatch, requires(MAP_TYPE m) { m.lower_bound(3); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" }, { 5, "five" } };

	auto it = map.lower_bound(3);

	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->first, 3);
	EXPECT_EQ(it->second, "three");
}

COND_TEST(LowerBound_NoExactMatch, requires(MAP_TYPE m) { m.lower_bound(4); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" }, { 5, "five" } };

	auto it = map.lower_bound(4);

	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->first, 5);
}

COND_TEST(LowerBound_BeyondLastElement, requires(MAP_TYPE m) { m.lower_bound(10); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto it = map.lower_bound(10);

	EXPECT_EQ(it, map.end());
}

COND_TEST(LowerBound_BeforeFirstElement, requires(MAP_TYPE m) { m.lower_bound(1); })
{
	MAP_TYPE map = { { 5, "five" }, { 10, "ten" } };

	auto it = map.lower_bound(1);

	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->first, 5);
}

COND_TEST(UpperBound_ExactMatch, requires(MAP_TYPE m) { m.upper_bound(3); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" }, { 5, "five" } };

	auto it = map.upper_bound(3);

	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->first, 5);
}

COND_TEST(UpperBound_NoExactMatch, requires(MAP_TYPE m) { m.upper_bound(4); })
{
	MAP_TYPE map = { { 1, "one" }, { 3, "three" }, { 5, "five" } };

	auto it = map.upper_bound(4);

	EXPECT_EQ(it->first, 5);
}

COND_TEST(UpperBound_BeyondLastElement, requires(MAP_TYPE m) { m.upper_bound(10); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto it = map.upper_bound(10);

	EXPECT_EQ(it, map.end());
}

COND_TEST(DefaultComparator_Less, std::is_default_constructible_v< typename MAP_TYPE::key_compare >)
{
	MAP_TYPE map;
	map[3] = "three";
	map[1] = "one";
	map[2] = "two";

	std::vector< KEY_T > keys;
	for (const auto& [k, v] : map)
	{
		keys.push_back(k);
	}

	EXPECT_EQ(keys, (std::vector< KEY_T >{ 1, 2, 3 }));
}

COND_TEST(CustomComparator_Greater, std::is_constructible_v< MAP_CG_TYPE, std::greater< KEY_T > >)
{
	MAP_CG_TYPE map;
	map[3] = "three";
	map[1] = "one";
	map[2] = "two";

	std::vector< KEY_T > keys;
	for (const auto& [k, v] : map)
	{
		keys.push_back(k);
	}

	EXPECT_EQ(keys, (std::vector< KEY_T >{ 3, 2, 1 }));
}

COND_TEST(KeyComp, requires(const MAP_TYPE m) { m.key_comp(); })
{
	MAP_TYPE map;

	auto comp = map.key_comp();

	EXPECT_TRUE(comp(1, 2));
	EXPECT_FALSE(comp(2, 1));
	EXPECT_FALSE(comp(1, 1));
}

COND_TEST(ExceptionTest_At_ThrowsOutOfRange, requires(MAP_TYPE m) { m.at(999); })
{
	MAP_TYPE map;
	map[1] = "one";

	EXPECT_THROW({ [[maybe_unused]] auto& val = map.at(999); }, std::out_of_range);

	EXPECT_EQ(map.size(), 1u);
	EXPECT_EQ(map.at(1), "one");
}

COND_TEST(
	ExceptionTest_CopyWithThrowingType_StrongGuarantee,
	requires(MAP_TOC_TYPE m) {
		m.emplace(1, NoCopy(1));
		m.emplace(2, std::declval< NoCopy >());
	})
{
	MAP_TOC_TYPE map;
	map.emplace(1, NoCopy(1));

	size_t original_size = map.size();

	NoCopy dummy(1);
	EXPECT_THROW(map.emplace(2, dummy), std::runtime_error);

	EXPECT_EQ(map.size(), original_size);
	EXPECT_EQ(map.count(1), 1u);
}

COND_TEST(
	SizeAndEmpty,
	requires(const MAP_TYPE m) {
		m.empty();
		m.size();
	})
{
	MAP_TYPE map;

	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);

	map[1] = "one";
	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.size(), 1u);

	map.clear();
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);
}

COND_TEST(MaxSize, requires(const MAP_TYPE m) { m.max_size(); })
{
	MAP_TYPE map;

	EXPECT_GT(map.max_size(), 1000000u);
}

COND_TEST(Swap, std::is_swappable_v< MAP_TYPE >)
{
	MAP_TYPE map1{ { 1, "one" }, { 2, "two" } };
	MAP_TYPE map2{ { 3, "three" }, { 4, "four" } };

	map1.swap(map2);

	EXPECT_EQ(map1.size(), 2u);
	EXPECT_EQ(map1.at(3), "three");
	EXPECT_EQ(map1.at(4), "four");

	EXPECT_EQ(map2.size(), 2u);
	EXPECT_EQ(map2.at(1), "one");
	EXPECT_EQ(map2.at(2), "two");
}

COND_TEST(RangeConstructor,
		  std::is_constructible_v< MAP_TYPE, std::vector< std::pair< int, std::string > >::iterator, std::vector< std::pair< int, std::string > >::iterator >)
{
	std::vector< std::pair< int, std::string > > vec = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	MAP_TYPE map(vec.begin(), vec.end());

	EXPECT_EQ(map.size(), 3u);
	EXPECT_EQ(map.at(1), "one");
	EXPECT_EQ(map.at(2), "two");
	EXPECT_EQ(map.at(3), "three");
}

COND_TEST(EqualRange, requires(MAP_TYPE m) { m.equal_range(2); })
{
	MAP_TYPE map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };

	auto [begin_it, end_it] = map.equal_range(2);

	ASSERT_NE(begin_it, end_it);
	EXPECT_EQ(begin_it->first, 2);
	EXPECT_EQ(std::distance(begin_it, end_it), 1);
}

COND_TEST(EqualRange_NonExisting, requires(MAP_TYPE m) { m.equal_range(2); })
{
	MAP_TYPE map = { { 1, "one" }, { 3, "three" } };

	auto [begin_it, end_it] = map.equal_range(2);

	EXPECT_EQ(begin_it, end_it);
	EXPECT_EQ(begin_it->first, 3);
}

COND_TEST(Equals, requires(MAP_TYPE m1, MAP_TYPE m2) { m1 == m2; })
{
	MAP_TYPE map1 = { { 1, "one" }, { 3, "three" } };
	MAP_TYPE map2 = { { 1, "one" }, { 3, "three" } };

	EXPECT_TRUE(map1 == map2);
}

COND_TEST(EqualsThreeWay, requires(MAP_TYPE m1, MAP_TYPE m2) { (m1 <=> m2) == 0; })
{
	MAP_TYPE map1 = { { 1, "one" }, { 3, "three" } };
	MAP_TYPE map2 = { { 1, "one" }, { 3, "three" } };

	EXPECT_TRUE((map1 <=> map2) == 0);
}

COND_TEST(NotEquals, requires(MAP_TYPE m1, MAP_TYPE m2) { m1 != m2; })
{
	MAP_TYPE map1 = { { 1, "one" }, { 3, "three" } };
	MAP_TYPE map2 = { { 1, "one" }, { 2, "three" } };

	EXPECT_TRUE(map1 != map2);
}

COND_TEST(NotEqualSize, requires(MAP_TYPE m1, MAP_TYPE m2) { m1 != m2; })
{
	MAP_TYPE map1 = { { 1, "one" }, { 3, "three" } };
	MAP_TYPE map2 = { { 1, "one" } };

	EXPECT_TRUE(map1 != map2);
}
