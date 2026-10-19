#pragma once

#include <utility>

#define EMPTY_TREE 0

enum Color
{
	RED,
	BLACK,
};

template< typename Key, typename T, typename Compare >
class RBTree
{
  private:
	struct Node
	{
		std::pair< const Key, T > value;
		Node* left;
		Node* right;
		Node* parent;
		Color color;

		Node(const std::pair< const Key, T >& new_value) :
			value(new_value), left(nullptr), right(nullptr), parent(nullptr), color(RED)
		{
		}
	};

  public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair< const Key, T >;
	using size_type = std::size_t;

	class const_iterator;
	class iterator
	{
	  public:
		using value_type = RBTree::value_type;

		iterator() : m_node(nullptr), m_tree(nullptr) {}
		iterator(Node* node, RBTree* tree) : m_node(node), m_tree(tree) {}

		value_type& operator*() { return m_node->value; }

		value_type* operator->() { return &m_node->value; }

		iterator& operator++()
		{
			m_node = m_tree->find_next_node(m_node);
			return *this;
		}

		iterator operator++(int)
		{
			iterator prev = *this;
			m_node = m_tree->find_next_node(m_node);
			return prev;
		}

		iterator& operator--()
		{
			if (m_node == nullptr)
			{
				m_node = m_tree->find_max_node(m_tree->m_root);
			}
			else
			{
				m_node = m_tree->find_prev_node(m_node);
			}

			return *this;
		}

		iterator operator--(int)
		{
			iterator prev = *this;
			if (m_node == nullptr)
			{
				m_node = m_tree->find_max_node(m_tree->m_root);
			}
			else
			{
				m_node = m_tree->find_prev_node(m_node);
			}
			return prev;
		}

		bool operator==(const iterator& other) const { return m_node == other.m_node; }

		bool operator!=(const iterator& other) const { return m_node != other.m_node; }

	  private:
		friend class const_iterator;

		Node* m_node;
		RBTree* m_tree;
	};

	class const_iterator
	{
	  public:
		using value_type = RBTree::value_type;

		const_iterator() : m_node(nullptr), m_tree(nullptr) {}
		const_iterator(Node* node, const RBTree* tree) : m_node(node), m_tree(tree) {}
		const_iterator(const iterator& other) : m_node(other.m_node), m_tree(other.m_tree) {}

		const value_type& operator*() const { return m_node->value; }

		const value_type* operator->() const { return &m_node->value; }

		const_iterator& operator++()
		{
			m_node = m_tree->find_next_node(m_node);
			return *this;
		}

		const_iterator operator++(int)
		{
			const_iterator prev = *this;
			m_node = m_tree->find_next_node(m_node);
			return prev;
		}

		const_iterator& operator--()
		{
			if (m_node == nullptr)
			{
				m_node = m_tree->find_max_node(m_tree->m_root);
			}
			else
			{
				m_node = m_tree->find_prev_node(m_node);
			}

			return *this;
		}

		const_iterator operator--(int)
		{
			const_iterator prev = *this;
			if (m_node == nullptr)
			{
				m_node = m_tree->find_max_node(m_tree->m_root);
			}
			else
			{
				m_node = m_tree->find_prev_node(m_node);
			}
			return prev;
		}

		bool operator==(const const_iterator& other) const { return m_node == other.m_node; }

		bool operator!=(const const_iterator& other) const { return m_node != other.m_node; }

	  private:
		Node* m_node;
		const RBTree* m_tree;
	};

	RBTree() : m_root(nullptr), m_size(EMPTY_TREE) {}
	RBTree(const Compare& comp) : m_root(nullptr), m_size(EMPTY_TREE), m_comp(comp) {}

	bool empty() const { return m_size == EMPTY_TREE; }

	size_type size() const { return m_size; }

	iterator begin() { return iterator(find_min_node(m_root), this); }

	const_iterator begin() const { return const_iterator(find_min_node(m_root), this); }

	iterator end() { return iterator(nullptr, this); }

	const_iterator end() const { return const_iterator(nullptr, this); }

	Compare key_comp() const { return m_comp; }

  private:
	Node* find_min_node(Node* node) const
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		while (node->left != nullptr)
		{
			node = node->left;
		}

		return node;
	}

	Node* find_max_node(Node* node) const
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		while (node->right != nullptr)
		{
			node = node->right;
		}

		return node;
	}

	Node* find_next_node(Node* node) const
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		if (node->right != nullptr)
		{
			return find_min_node(node->right);
		}

		Node* parent = node->parent;

		while (parent != nullptr && node == parent->right)
		{
			node = parent;
			parent = parent->parent;
		}

		return parent;
	}

	Node* find_prev_node(Node* node) const
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		if (node->left != nullptr)
		{
			return find_max_node(node->left);
		}

		Node* parent = node->parent;

		while (parent != nullptr && node == parent->left)
		{
			node = parent;
			parent = parent->parent;
		}

		return parent;
	}

	Node* m_root;
	size_type m_size;
	Compare m_comp;
};

#undef EMPTY_TREE
