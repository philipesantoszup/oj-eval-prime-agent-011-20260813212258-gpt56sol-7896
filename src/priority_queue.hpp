#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * A max-priority queue implemented as a leftist heap.
 *
 * Meld follows only right spines, so push, pop and merge are O(log n)
 * (merge is O(log(n + m))).  In meld all comparisons are completed on the
 * way down and pointers are changed only while the recursion unwinds.  Thus
 * a throwing comparator leaves every input tree unchanged.
 */
template<typename T, class Compare = std::less<T> >
class priority_queue {
private:
	struct node {
		T value;
		node *left;
		node *right;
		size_t rank;

		explicit node(const T &v)
			: value(v), left(NULL), right(NULL), rank(1) {}
	};

	node *root;
	size_t count;
	Compare compare;

	static size_t get_rank(const node *p) {
		return p == NULL ? 0 : p->rank;
	}

	/*
	 * No pointer is modified before the recursive call succeeds.  Once that
	 * call has succeeded there are no more potentially-throwing operations.
	 */
	node *meld(node *a, node *b) {
		if (a == NULL) return b;
		if (b == NULL) return a;
		if (compare(a->value, b->value)) {
			node *tmp = a;
			a = b;
			b = tmp;
		}
		node *new_right = meld(a->right, b);
		a->right = new_right;
		if (get_rank(a->left) < get_rank(a->right)) {
			node *tmp = a->left;
			a->left = a->right;
			a->right = tmp;
		}
		a->rank = get_rank(a->right) + 1;
		return a;
	}

	/* Constant-extra-space destruction also handles a degenerate left spine. */
	static void destroy(node *p) {
		while (p != NULL) {
			if (p->left != NULL) {
				node *next = p->left;
				p->left = next->right;
				next->right = p;
				p = next;
			} else {
				node *next = p->right;
				delete p;
				p = next;
			}
		}
	}

	/*
	 * Iterate down left links (which may form a linear chain).  Only right
	 * subtrees are recursive; their height is logarithmic in a leftist heap.
	 */
	static node *clone(const node *p) {
		if (p == NULL) return NULL;
		node *result = NULL;
		node *tail = NULL;
		try {
			while (p != NULL) {
				node *copy = new node(p->value);
				if (result == NULL) result = copy;
				else tail->left = copy;
				tail = copy;
				copy->rank = p->rank;
				copy->right = clone(p->right);
				p = p->left;
			}
		} catch (...) {
			destroy(result);
			throw;
		}
		return result;
	}

public:
	priority_queue() : root(NULL), count(0), compare() {}

	priority_queue(const priority_queue &other)
		: root(NULL), count(other.count), compare(other.compare) {
		root = clone(other.root);
	}

	~priority_queue() {
		destroy(root);
	}

	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		/* Clone first, giving the contained values the strong guarantee. */
		node *new_root = clone(other.root);
		try {
			compare = other.compare;
		} catch (...) {
			destroy(new_root);
			throw;
		}
		destroy(root);
		root = new_root;
		count = other.count;
		return *this;
	}

	const T &top() const {
		if (root == NULL) throw container_is_empty();
		return root->value;
	}

	void push(const T &e) {
		node *added = new node(e);
		try {
			root = meld(root, added);
		} catch (...) {
			delete added;
			throw runtime_error();
		}
		++count;
	}

	void pop() {
		if (root == NULL) throw container_is_empty();
		node *old_root = root;
		node *new_root;
		try {
			new_root = meld(old_root->left, old_root->right);
		} catch (...) {
			throw runtime_error();
		}
		root = new_root;
		delete old_root;
		--count;
	}

	size_t size() const {
		return count;
	}

	bool empty() const {
		return count == 0;
	}

	void merge(priority_queue &other) {
		if (this == &other) return;
		node *new_root;
		try {
			new_root = meld(root, other.root);
		} catch (...) {
			throw runtime_error();
		}
		root = new_root;
		count += other.count;
		other.root = NULL;
		other.count = 0;
	}
};

} // namespace sjtu

#endif
