#pragma once

#include <utility>

#define EMPTY_TREE 0
#define RIGHT_SUBTREE_DIFF 2
#define LEFT_SUBTREE_DIFF -2

template< typename Key, typename T, typename Compare >
class AVLTree
{
  private:
	struct Node
	{
		std::pair< const Key, T > value;
		Node* left;
		Node* right;
		Node* parent;
		int height;

		Node(const std::pair< const Key, T >& new_value) :
			value(new_value), left(nullptr), right(nullptr), parent(nullptr), height(1)
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
		using value_type = AVLTree::value_type;

		iterator() : m_node(nullptr), m_tree(nullptr) {}
		iterator(Node* node, AVLTree* tree) : m_node(node), m_tree(tree) {}

		value_type& operator*() { return m_node->value; }

		value_type* operator->() { return &m_node->value; }

		iterator& operator++()
		{
			m_node = m_tree->find_next_node(m_node);
			return (*this);
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

			return (*this);
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
		AVLTree* m_tree;
	};

	class const_iterator
	{
	  public:
		using value_type = AVLTree::value_type;

		const_iterator() : m_node(nullptr), m_tree(nullptr) {}
		const_iterator(Node* node, const AVLTree* tree) : m_node(node), m_tree(tree) {}
		const_iterator(const iterator& other) : m_node(other.m_node), m_tree(other.m_tree) {}

		const value_type& operator*() const { return m_node->value; }

		const value_type* operator->() const { return &m_node->value; }

		const_iterator& operator++()
		{
			m_node = m_tree->find_next_node(m_node);
			return (*this);
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

			return (*this);
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
		const AVLTree* m_tree;
	};

	AVLTree() : m_root(nullptr), m_size(EMPTY_TREE) {}
	AVLTree(const Compare& comp) : m_root(nullptr), m_size(EMPTY_TREE), m_comp(comp) {}

	bool empty() const { return m_size == EMPTY_TREE; }

	size_type size() const { return m_size; }

	iterator begin() { return iterator(find_min_node(m_root), this); }

	const_iterator begin() const { return const_iterator(find_min_node(m_root), this); }

	iterator end() { return iterator(nullptr, this); }

	const_iterator end() const { return const_iterator(nullptr, this); }

	Compare key_comp() const { return m_comp; }

	std::pair< iterator, bool > insert(const value_type& value)
	{
		if (m_root == nullptr)
		{
			m_root = new Node(value);
			m_size++;

			return std::pair< iterator, bool >(iterator(m_root, this), true);
		}

		Node* current = m_root;
		Node* parent = nullptr;

		while (current != nullptr)
		{
			parent = current;

			if (m_comp(value.first, current->value.first))
			{
				current = current->left;
			}
			else if (m_comp(current->value.first, value.first))
			{
				current = current->right;
			}
			else
			{
				return std::pair< iterator, bool >(iterator(current, this), false);
			}
		}

		Node* new_node = new Node(value);
		new_node->parent = parent;

		if (m_comp(value.first, parent->value.first))
		{
			parent->left = new_node;
		}
		else
		{
			parent->right = new_node;
		}

		m_size++;
		balance_all_nodes(parent);

		return std::pair< iterator, bool >(iterator(new_node, this), true);
	}

  private:
	int get_height(Node* node) const { return node == nullptr ? 0 : node->height; }

	int balance_factor(Node* node) const { return get_height(node->right) - get_height(node->left); }

	void fix_height(Node* node)
	{
		int left_height = get_height(node->left);
		int right_height = get_height(node->right);

		if (left_height > right_height)
		{
			node->height = left_height + 1;
		}
		else
		{
			node->height = right_height + 1;
		}
	}

	Node* rotate_right(Node* node)
	{
		Node* q_node = node->left;
		Node* rest_node = q_node->right;

		node->left = rest_node;

		if (rest_node != nullptr)
		{
			rest_node->parent = node;
		}

		q_node->parent = node->parent;

		if (node->parent == nullptr)
		{
			m_root = q_node;
		}
		else if (node == node->parent->left)
		{
			node->parent->left = q_node;
		}
		else
		{
			node->parent->right = q_node;
		}

		q_node->right = node;
		node->parent = q_node;

		fix_height(node);
		fix_height(q_node);

		return q_node;
	}

	Node* rotate_left(Node* node)
	{
		Node* q_node = node->right;
		Node* rest_node = q_node->left;

		node->right = rest_node;

		if (rest_node != nullptr)
		{
			rest_node->parent = node;
		}

		q_node->parent = node->parent;

		if (node->parent == nullptr)
		{
			m_root = q_node;
		}
		else if (node == node->parent->left)
		{
			node->parent->left = q_node;
		}
		else
		{
			node->parent->right = q_node;
		}

		q_node->left = node;
		node->parent = q_node;
		fix_height(node);
		fix_height(q_node);

		return q_node;
	}

	Node* balance_node(Node* node)
	{
		fix_height(node);
		if (balance_factor(node) == RIGHT_SUBTREE_DIFF)
		{
			if (balance_factor(node->right) < 0)
			{
				rotate_right(node->right);
			}

			return rotate_left(node);
		}

		if (balance_factor(node) == LEFT_SUBTREE_DIFF)
		{
			if (balance_factor(node->left) > 0)
			{
				rotate_left(node->left);
			}

			return rotate_right(node);
		}

		return node;
	}

	void balance_all_nodes(Node* node)
	{
		while (node != nullptr)
		{
			Node* par = node->parent;
			balance_node(node);
			node = par;
		}
	}

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
#undef RIGHT_SUBTREE_DIFF
#undef LEFT_SUBTREE_DIFF
