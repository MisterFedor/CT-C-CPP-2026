
using K = typename MAP_TYPE::key_type;
using M = typename MAP_TYPE::mapped_type;
using V = typename MAP_TYPE::value_type;
using Sz = typename MAP_TYPE::size_type;
using It = typename MAP_TYPE::iterator;
using CIt = typename MAP_TYPE::const_iterator;

TEST(MapNamedRequirements, TypeAliases)
{
	CHECK_REQ(std::is_same_v< typename MAP_TYPE::key_type, KEY_T >);
	CHECK_REQ(std::is_same_v< typename MAP_TYPE::mapped_type, VALUE_T >);
	CHECK_REQ(std::is_same_v< typename MAP_TYPE::value_type, std::pair< const KEY_T, VALUE_T > >);
	CHECK_REQ(std::is_same_v< typename MAP_TYPE::key_compare, std::less< KEY_T > >);
	CHECK_REQ(std::is_same_v< typename MAP_TYPE::size_type, std::size_t >);
	CHECK_REQ(std::is_signed_v< typename MAP_TYPE::difference_type >);
}

TEST(MapNamedRequirements, IteratorConcepts)
{
	using It = typename MAP_TYPE::iterator;

	CHECK_REQ(std::bidirectional_iterator< It >);
	CHECK_REQ(std::input_iterator< It >);
	CHECK_REQ(std::forward_iterator< It >);
}

TEST(MapNamedRequirements, ConstIteratorConcepts)
{
	using It = typename MAP_TYPE::iterator;
	using CIt = typename MAP_TYPE::const_iterator;

	CHECK_REQ(std::bidirectional_iterator< CIt >);
	CHECK_REQ(std::is_convertible_v< It, CIt >);
	CHECK_REQ(std::input_iterator< CIt >);
}

TEST(MapNamedRequirements, RuleOfFiveAndSwappable)
{
	CHECK_REQ(std::is_default_constructible_v< MAP_TYPE >);
	CHECK_REQ(std::is_copy_constructible_v< MAP_TYPE >);
	CHECK_REQ(std::is_move_constructible_v< MAP_TYPE >);
	CHECK_REQ(std::is_copy_assignable_v< MAP_TYPE >);
	CHECK_REQ(std::is_move_assignable_v< MAP_TYPE >);
	CHECK_REQ(std::is_destructible_v< MAP_TYPE >);
	CHECK_REQ(std::is_swappable_v< MAP_TYPE >);
}

TEST(MapNamedRequirements, MemberCapacityFunctionsExistence)
{
	CHECK_REQ(requires(MAP_TYPE m) {
		{ m.empty() } -> std::convertible_to< bool >;
		{ m.size() } -> std::convertible_to< Sz >;
		{ m.max_size() } -> std::convertible_to< Sz >;
	});
}

TEST(MapNamedRequirements, MemberIteratorsFunctionsExistence)
{
	CHECK_REQ(requires(MAP_TYPE m, It it) {
		{ m.begin() } -> std::same_as< It >;
		{ m.end() } -> std::same_as< It >;
	});
}

TEST(MapNamedRequirements, MemberConstIteratorsFunctionsExistence)
{
	CHECK_REQ(requires(const MAP_TYPE cm, CIt it) {
		{ cm.begin() } -> std::same_as< CIt >;
		{ cm.end() } -> std::same_as< CIt >;
		{ cm.cbegin() } -> std::same_as< CIt >;
		{ cm.cend() } -> std::same_as< CIt >;
	});
}

TEST(MapNamedRequirements, MemberModifiersFunctionsExistence)
{
	CHECK_REQ(requires(MAP_TYPE m, K k, V v, It it) {
		{ m.insert(v) } -> std::same_as< std::pair< It, bool > >;
		{ m.erase(it) } -> std::same_as< It >;
		{ m.erase(k) } -> std::same_as< Sz >;
		{ m.clear() } -> std::same_as< void >;
	});
}

TEST(MapNamedRequirements, MemberLookupFunctionsExistence)
{
	CHECK_REQ(requires(MAP_TYPE m, const MAP_TYPE cm, K k) {
		{ m.find(k) } -> std::same_as< It >;
		{ cm.find(k) } -> std::same_as< CIt >;
		{ m.count(k) } -> std::same_as< Sz >;
		{ m.lower_bound(k) } -> std::same_as< It >;
		{ m.upper_bound(k) } -> std::same_as< It >;
		{ m.equal_range(k) } -> std::same_as< std::pair< It, It > >;
	});
}

TEST(MapNamedRequirements, MemberObserversFunctionsExistence)
{
	CHECK_REQ(requires(MAP_TYPE m) {
		{ m.key_comp() } -> std::invocable< K, K >;
		{ m.value_comp() } -> std::invocable< V, V >;
	});
}

TEST(MapNamedRequirements, MemberSwapFunctionExistence)
{
	CHECK_REQ(requires(MAP_TYPE m) { m.swap(m); });
}

TEST(MapNamedRequirements, MapSpecificOperators)
{
	CHECK_REQ(requires(MAP_TYPE m, const MAP_TYPE cm, K k) {
		{ m[k] } -> std::same_as< M & >;
		{ m.at(k) } -> std::same_as< M & >;
		{ cm.at(k) } -> std::same_as< const M & >;
	});
}
