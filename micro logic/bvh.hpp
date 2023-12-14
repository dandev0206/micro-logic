#pragma once

#include "aabb.hpp"

#include <vector>
#include <stack>

#define BVH_CONTINUE return false
#define BVH_BREAK    return true

template <class Ty>
struct _BVH_Node {
	using key_type      = AABB;
	using mapped_type   = Ty;
	using value_type    = std::pair<AABB, Ty>;
	using pointer       = value_type*;
	using const_pointer = const value_type*;

	_BVH_Node() noexcept :
		parent(nullptr),
		childs(),
		value()
	{}

	_BVH_Node(const AABB& aabb, const mapped_type& value) noexcept :
		parent(nullptr),
		childs(),
		value(aabb, value)
	{}

	_BVH_Node(const AABB& aabb, mapped_type&& value) noexcept :
		parent(nullptr),
		childs(),
		value(std::move(aabb), std::move(value))
	{}

	~_BVH_Node() {};

	bool is_branch() const {
		return childs[0] && childs[1];
	}

	bool is_leaf() const {
		return !is_branch();
	}

	_BVH_Node* erase_and_merge(int32_t child_idx) {
		if (parent->childs[0] == this) {
			parent->childs[0] = childs[!child_idx];
			parent->childs[0]->parent = parent;
		} else {
			parent->childs[1] = childs[!child_idx];
			parent->childs[1]->parent = parent;
		}

		return childs[child_idx];
	}

	void update_AABB() {
		aabb = childs[0]->aabb.union_of(childs[1]->aabb);
	}

	_BVH_Node* parent;
	_BVH_Node* childs[2];
	union {
		AABB       aabb;
		value_type value;
	};
};

template <class Ty>
class _BVH_Const_Iterator {	
	template <class>
	friend class BVH;

public:
	using iterator_category = std::forward_iterator_tag;
    using value_type        = std::pair<AABB, Ty>;
    using pointer           = const value_type*;
    using reference         = const value_type&;

	_BVH_Const_Iterator() :
		ptr(nullptr)
	{}

	_BVH_Const_Iterator(const _BVH_Const_Iterator& rhs) :
		ptr(rhs.ptr)
	{}

	_BVH_Const_Iterator(_BVH_Node<Ty>* ptr) :
		ptr(ptr)
	{}

	reference operator*() const noexcept {
		return ptr->value;
	}

	pointer operator->() const noexcept {
		return &(ptr->value);
	}

	_BVH_Const_Iterator& operator++() noexcept {
		auto* parent = ptr->parent;

		if (parent == nullptr) {
			ptr = nullptr;
		} else {
			if (parent->childs[0] == ptr) {
				ptr = parent->childs[1];
			} else {
				do {
					ptr    = parent;
					parent = parent->parent;

					if (parent == nullptr) {
						ptr = nullptr;
						return *this;
					}
				} while (parent->childs[0] != ptr);
			}

			ptr = parent->childs[1];
			while (!ptr->is_leaf())
				ptr = ptr->childs[0];
		}

		return *this;
	}

	_BVH_Const_Iterator operator++(int) noexcept {
		_BVH_Const_Iterator temp = *this;
		++*this;
		return temp;
	}

	bool operator==(const _BVH_Const_Iterator& rhs) const noexcept {
		return ptr == rhs.ptr;
	}

	bool operator!=(const _BVH_Const_Iterator& rhs) const noexcept {
		return !(*this == rhs);
	}

protected:
	_BVH_Node<Ty>* ptr;
};

template <class Ty>
class _BVH_Iterator : public _BVH_Const_Iterator<Ty> {
	template <class>
	friend class BVH;

	using _myBase = _BVH_Const_Iterator<Ty>;
public:
	using iterator_category = std::forward_iterator_tag;
    using value_type        = std::pair<AABB, Ty>;
    using pointer           = value_type*;
    using reference         = value_type&;

	using _myBase::_myBase;

	reference operator*() noexcept {
		return _myBase::ptr->value;
	}

	pointer operator->() noexcept {
		return &(_myBase::ptr->value);
	}

	_BVH_Iterator& operator++() noexcept {
		_myBase::operator++();
		return *this;
	}

	_BVH_Iterator operator++(int) noexcept {
		_BVH_Const_Iterator temp = *this;
		_myBase::operator++();
		return temp;
	}
};

template <class Ty>
class BVH {
public:
	using point_type     = vec2;
	using rect_type      = Rect;
	using key_type       = AABB;
	using mapped_type    = Ty;
	using value_type     = std::pair<AABB, Ty>;
	using size_type      = size_t;
	using pointer        = value_type*;
	using const_pointer  = const value_type *;
	using reference      = value_type&;
	using iterator       = _BVH_Iterator<Ty>;
	using const_iterator = _BVH_Const_Iterator<Ty>;

	BVH() noexcept :
		root(nullptr),
		node_size(0) 
	{}

	BVH(BVH&& rhs) noexcept :
		root(std::exchange(rhs.root, nullptr)),
		node_size(std::exchange(rhs.node_size, 0))
	{}

	template <class Iter>
	BVH(Iter first, Iter last) {
		insert(first, last);
	}
	
	BVH& operator=(BVH&& rhs) noexcept {
		root      = std::exchange(rhs.root, nullptr);
		node_size = std::exchange(rhs.node_size, 0);
	}

private:
	_BVH_Node<Ty>* _find_best(const AABB& aabb) {
		auto   cost_best = aabb.union_of(root->aabb).area();
		auto** node_best = &root;

		find_stack.emplace_back(root, 0.f);

		while (!find_stack.empty()) {
			auto* curr_node   = find_stack.back().first;
			auto cost_inherit = find_stack.back().second;

			find_stack.pop_back();

			float cost_direct = aabb.union_of(curr_node->aabb).area();
			float cost_total  = cost_direct + cost_inherit;

			if (cost_total < cost_best) {
				cost_best = cost_total;
				node_best = &curr_node;
			}

			cost_inherit += cost_direct - curr_node->aabb.area();

			float cost_lowerbound = aabb.area() + cost_inherit;

			if (cost_lowerbound < cost_best) {
				if (!curr_node->is_leaf()) {
					find_stack.emplace_back(curr_node->childs[0], cost_inherit);
					find_stack.emplace_back(curr_node->childs[1], cost_inherit);
				}
			}
		}

		find_stack.clear();

		return *node_best;
	}

	void _refit(_BVH_Node<Ty>* node) {
		while (node) {
			node->update_AABB();
			node = node->parent;
		}
	}

	iterator _insert_one_impl(_BVH_Node<Ty>* new_node) {
		if (empty()) {
			new_node->parent = nullptr;
			root      = new_node;
		} else {
			auto* sibling    = _find_best(new_node->aabb);
			auto* old_parent = sibling->parent;
			auto* new_parent = new _BVH_Node<Ty>();

			sibling->parent       = new_parent;
			new_node->parent      = new_parent;
			new_parent->parent    = old_parent;
			new_parent->childs[0] = sibling;
			new_parent->childs[1] = new_node;
			new_parent->update_AABB();

			if (old_parent != nullptr) {
				if (old_parent->childs[0] == sibling)
					old_parent->childs[0] = new_parent;
				else
					old_parent->childs[1] = new_parent;
			} else {
				root = new_parent;
			}

			_refit(old_parent);
		}

		++node_size;
		return iterator(new_node);
	}

public:
	iterator insert(const value_type& val) {
		auto* new_node = new _BVH_Node<Ty>(val.first, val.second);
		return _insert_one_impl(new_node);
	}

	iterator insert(value_type&& val) {
		auto* new_node = new _BVH_Node<Ty>(val.first, std::move(val.second));
		return _insert_one_impl(new_node);
	}

	iterator insert(const AABB& aabb, const Ty& item) {
		auto* new_node = new _BVH_Node<Ty>(aabb, item);
		return _insert_one_impl(new_node);
	}

	iterator insert(const AABB& aabb, Ty&& item) {
		auto* new_node = new _BVH_Node<Ty>(aabb, std::move(item));
		return _insert_one_impl(new_node);
	}

	template <class Iter>
	void insert(Iter first, Iter last) {
		static_assert("not implemented");
	}

	template <class... Args>  
	iterator emplace(const AABB& aabb, Args&&... args) {
		return _insert_one_impl(new value_type(aabb, Ty(std::forward<Args>(args)...)));
	}

private:
	_BVH_Node<Ty>* _erase_impl(iterator iter) {
		_BVH_Node<Ty>* erased = nullptr;

		auto* node   = iter.ptr;
		auto* parent = node->parent;

		--node_size;

		if (parent != nullptr) {
			auto branch_parent = parent->parent;

			if (branch_parent != nullptr) {
				if (parent->childs[0] == node) {
					erased = parent->erase_and_merge(0);
				} else {
					erased = parent->erase_and_merge(1);
				}

				delete parent;

				_refit(branch_parent);
			} else {
				if (parent->childs[0] == node) {
					root = parent->childs[1];
					erased = std::exchange(parent->childs[0], nullptr);
				} else {
					root = parent->childs[0];
					erased = std::exchange(parent->childs[1], nullptr);
				}

				delete std::exchange(root->parent, nullptr);
			}
		} else {
			erased = std::exchange(root, nullptr);
		}
		
		return erased;
	}

public:
	void erase(iterator iter) {
		delete _erase_impl(iter);
	}

	template <class Iter>
	void erase(Iter first, Iter last) {
		for (auto iter = first; iter != last; ++iter)
			erase(iter);
	}

	void update_element(iterator iter, const AABB aabb) {
		auto* node = _erase_impl(iter);
		node->aabb = aabb;
		_insert_one_impl(node);
	}

	void swap(BVH& rhs) noexcept {
		std::swap(root, rhs.root);
		std::swap(node_size, rhs.node_size);
	}

	void clear() {
		if (!root) return;

		stack.push_back(root);

		while (!stack.empty()) {
			auto* node = stack.back();
			stack.pop_back();

			if (!node->is_leaf()) {
				stack.push_back(node->childs[0]);
				stack.push_back(node->childs[1]);
			}

			delete node;
		}

		node_size = 0;
	}

	template <class Pred>
	bool query(const point_type& pos, Pred func) {
		if (!root || !root->aabb.contain(pos)) {
			return false;
		}

		stack.push_back(root);

		while (!stack.empty()) {
			auto* node = stack.back();
			stack.pop_back();

			if (node->is_leaf()) {
				if (func(iterator(node))) {
					stack.clear();
					return true;
				}
			} else {
				if (node->childs[0]->aabb.contain(pos))
					stack.push_back(node->childs[0]);
				if (node->childs[1]->aabb.contain(pos))
					stack.push_back(node->childs[1]);
			}
		}

		return false;
	}

	template <class Pred>
	bool query(const rect_type& rect, Pred func) {
		if (!root || !root->aabb.overlap(rect)) {
			return false;
		}

		stack.push_back(root);

		while (!stack.empty()) {
			auto* node = stack.back();
			stack.pop_back();

			if (node->is_leaf()) {
				if (func(iterator(node))) {
					stack.clear();
					return true;
				}
			} else {
				if (node->childs[0]->aabb.overlap(rect))
					stack.push_back(node->childs[0]);
				if (node->childs[1]->aabb.overlap(rect))
					stack.push_back(node->childs[1]);
			}
		}

		return false;
	}

	template <class Pred>
	void traverse(Pred func) {
		if (!root) return;

		std::stack<std::pair<_BVH_Node<Ty>*, uint32_t>> stack;

		stack.emplace(root, 0);

		while (!stack.empty()) {
			auto* node  = stack.top().first;
			auto  level = stack.top().second;

			stack.pop();
			
			if (func(node->aabb, level) && !node->is_leaf()) {
				stack.emplace(node->childs[0], level + 1);
				stack.emplace(node->childs[1], level + 1);
			}
		}
	}

private:
	_BVH_Node<Ty>* _find_first() const {
		if (!root) return nullptr;

		auto* node = root;

		while (!node->is_leaf())
			node = node->childs[0];

		return node;
	}

public:
	iterator begin() {
		return iterator(_find_first());
	}

	const_iterator begin() const {
		return const_iterator(_find_first());
	}

	iterator end() {
		return iterator(nullptr);
	}

	const_iterator end() const {
		return const_iterator(nullptr);
	}

	const_iterator cbegin() const {
		return const_iterator(_find_first());
	}

	const_iterator cend() const {
		return const_iterator(nullptr);
	}

	bool empty() const {
		return !root;
	}

	size_type size() const {
		return node_size;
	}

private:
	_BVH_Node<Ty>* root;
	size_type node_size;

	std::vector<std::pair<_BVH_Node<Ty>*, float>> find_stack;
	std::vector<_BVH_Node<Ty>*> stack;
};