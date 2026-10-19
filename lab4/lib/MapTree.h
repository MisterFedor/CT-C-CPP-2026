#pragma once

#include "AVLTree.hpp"
#include "RBTree.hpp"

#include <cstdint>
#include <functional>

template< typename Key, typename T, template< typename TreeKey, typename TreeT, typename TreeCompare > class Tree, typename Compare = std::less< Key > >
class MapTree
{
  public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair< const Key, T >;
	using key_compare = Compare;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using tree_type = Tree< Key, T, Compare >;
	using iterator = typename tree_type::iterator;
	using const_iterator = typename tree_type::const_iterator;

	MapTree() {}
	MapTree(const key_compare& comp) : m_tree(comp) {}

	bool empty() const { return m_tree.empty(); }

	size_type size() const { return m_tree.size(); }

	key_compare key_comp() const { return m_tree.key_comp(); }

	iterator begin() { return m_tree.begin(); }

	const_iterator begin() const { return m_tree.begin(); }

	iterator end() { return m_tree.end(); }

	const_iterator end() const { return m_tree.end(); }

	const_iterator cbegin() const { return m_tree.begin(); }

	const_iterator cend() const { return m_tree.end(); }

	size_type max_size() const { return SIZE_MAX / sizeof(value_type); }

  private:
	tree_type m_tree;
};
