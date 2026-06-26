#ifndef MAP_CTL_H
#define MAP_CTL_H

#include <iostream>
#include <utility>
#include <algorithm>

namespace CTL {
    enum COLOR{
		BLACK_t,
		RED_t
	};
	template <class V>
	struct RBTreeNode{
		RBTreeNode<V>* _parent; //父节点
		RBTreeNode<V>* _left; //左孩子
		RBTreeNode<V>* _right; //右孩子
		V _val;
		COLOR _color; //颜色
		explicit RBTreeNode(const V& val = V())
			:_parent(nullptr)
			, _left(nullptr)
			, _right(nullptr)
			, _val(val)
			, _color(RED_t) {

		}
	};
	template <class K, class V, class KeyOfValue>
	class RBTree{
	public:
		typedef RBTreeNode<V> Node;
		RBTree():_header(new Node){
			_header->_left = _header->_right = _header;
		}
		~RBTree() {
			clear();
			delete _header;
			_header = nullptr;
		}
		bool insert(const V& val){
			if (_header->_parent == nullptr){
				Node* root = new Node(val);
				_header->_parent = root;
				root->_parent = _header;
				_header->_left = _header->_right = root;
				//根节点为黑色
				root->_color = BLACK_t;
				return true;
			}

			Node* cur = _header->_parent;
			Node* parent = nullptr;

			KeyOfValue kov;
			//1.寻找到要插入的结点的位置
			while (cur){
				parent = cur;
				if (kov(cur->_val) == kov(val)) {
					return false;
				}
				else if (kov(cur->_val) > kov(val)) {
					cur = cur->_left;
				}
				else {
					cur = cur->_right;
				}
			}
			//2.创建节点
			cur = new Node(val);
			if (kov(parent->_val) > kov(cur->_val)) {
				parent->_left = cur;
			}
			else {
				parent->_right = cur;
			}
			cur->_parent = parent;

			//3.颜色的修改或者结构的调整
			while (cur != _header->_parent && cur->_parent->_color == RED_t){ //不为根且存在连续红色，则需要调整
				parent = cur->_parent;
				Node* gfather = parent->_parent;

				if (gfather->_left == parent){
					Node* uncle = gfather->_right;
					//情况1.uncle存在且为红
					if (uncle && uncle->_color == RED_t){
						parent->_color = uncle->_color = BLACK_t;
						gfather->_color = RED_t;
						//向上追溯
						cur = gfather;
					}
					else{
						if (parent->_right == cur){ //情况3
							RotateL(parent);
							swap(cur, parent);
						}
						//情况2.uncle不存在或者uncle为黑
						RotateR(gfather);
						parent->_color = BLACK_t;
						gfather->_color = RED_t;
						break;
					}
				}
				else{
					Node* uncle = gfather->_left;
					if (uncle && uncle->_color == RED_t){
						parent->_color = uncle->_color = BLACK_t;
						gfather->_color = RED_t;
						//向上追溯
						cur = gfather;
					}
					else{
						if (parent->_left == cur){
							RotateR(parent);
							swap(cur, parent);
						}

						RotateL(gfather);
						parent->_color = BLACK_t;
						gfather->_color = RED_t;
						break;
					}
				}
			}

			//根节点为黑色
			_header->_parent->_color = BLACK_t;
			//更新头结点的左右指向
			_header->_left = leftMost();
			_header->_right = rightMost();
			return true;
		}
		void RotateL(Node* parent){
			Node* subR = parent->_right;
			Node* subRL = subR->_left;

			parent->_right = subRL;
			if (subRL) {
				subRL->_parent = parent;
			}
			if (parent == _header->_parent){
				_header->_parent = subR;
				subR->_parent = _header;
			}
			else{
				Node* gfather = parent->_parent;
				if (gfather->_left == parent) {
					gfather->_left = subR;
				}
				else {
					gfather->_right = subR;
				}
				subR->_parent = gfather;
			}
			subR->_left = parent;
			parent->_parent = subR;
		}
		void RotateR(Node* parent){
			Node* subL = parent->_left;
			Node* subLR = subL->_right;

			parent->_left = subLR;
			if (subLR) {
				subLR->_parent = parent;
			}

			if (parent == _header->_parent){
				_header->_parent = subL;
				subL->_parent = _header;
			}
			else{
				Node* gfather = parent->_parent;
				if (gfather->_left == parent) {
					gfather->_left = subL;
				}
				else {
					gfather->_right = subL;
				}
				subL->_parent = gfather;
			}
			subL->_right = parent;
			parent->_parent = subL;
		}
		Node* leftMost() const {
			Node* cur = _header->_parent;
			while (cur && cur->_left){
				cur = cur->_left;
			}
			return cur;
		}
		Node* rightMost() const {
			Node* cur = _header->_parent;
			while (cur && cur->_right){
				cur = cur->_right;
			}
			return cur;
		}
		Node* leftMostPublic() const {
			Node* node = leftMost();
			if (node == _header) {
				return nullptr;
			}
			return node;
		}
		Node* getRoot() const {
			return _header->_parent;
		}
		Node* findNode(const K& key) {
			Node* cur = getRoot();
			KeyOfValue kov;
			while (cur) {
				if (kov(cur->_val) == key) {
					return cur;
				}
				else if (kov(cur->_val) > key) {
					cur = cur->_left;
				}
				else {
					cur = cur->_right;
				}
			}
			return nullptr;
		}
		Node* getHeader() const {
			return _header;
		}
		bool erase(const K& key) {
			Node* node = findNode(key);
			if (!node) return false;

			// 实际删除节点的逻辑
			deleteNode(node);
			return true;
		}
		void clear() {
			destroy(_header->_parent);
			_header->_parent = nullptr;
			_header->_left = _header->_right = _header;
		}
	private:
		void destroy(Node* node) {
			if (!node || node == _header) return;
			std::stack<Node*> nodes_to_delete;
			nodes_to_delete.push(node);
			while (!nodes_to_delete.empty()) {
				Node* current = nodes_to_delete.top();
				nodes_to_delete.pop();
				if (current->_left && current->_left != _header && current->_left != nullptr) {
					nodes_to_delete.push(current->_left);
				}
				if (current->_right && current->_right != _header && current->_right != nullptr) {
					nodes_to_delete.push(current->_right);
				}
				delete current;
				current = nullptr;
			}
		}
		void destroy_t(Node* node) {
			if (node && node != _header) {
				destroy(node->_left);
				destroy(node->_right);
				delete node;
				node = nullptr;
			}
		}
		void deleteNode(Node* node) {
			// 简化的删除实现（实际红黑树删除更复杂）
			if (node->_left && node->_right) {
				// 找到中序后继
				Node* successor = node->_right;
				while (successor->_left) {
					successor = successor->_left;
				}

				// 交换值
				std::swap(node->_val, successor->_val);

				// 删除后继节点
				deleteNode(successor);
				return;
			}

			// 节点最多只有一个子节点
			Node* child = node->_left ? node->_left : node->_right;
			if (!node->_parent) {
				// 删除根节点
				_header->_parent = child;
			} else if (node == node->_parent->_left) {
				node->_parent->_left = child;
			} else {
				node->_parent->_right = child;
			}

			if (child) {
				child->_parent = node->_parent;
			}

			delete node;

			// 更新边界节点
			if (_header->_parent) {
				_header->_left = leftMost();
				_header->_right = rightMost();
			} else {
				_header->_left = _header->_right = _header;
			}
		}
	public:
	private:
		Node* _header = nullptr;
	};
    template<typename K, typename V>
    class Map_CTL {
    	struct MapKeyOfValue{
    		const K& operator()(const std::pair<K, V>& val){
    			return val.first;
    		}
    	};
    	typedef RBTree<K, std::pair<K, V>, MapKeyOfValue> rbt;
    	rbt _rbt;
    public:
    	class iterator {
    		typename rbt::Node* _node;
    		const Map_CTL* _map;
    		const rbt* _tree;
    	public:
    		iterator(typename rbt::Node* node, const rbt* tree)
			    : _node(node), _map(nullptr), _tree(tree) {
		    }
    		iterator& operator++() {
    			if (_node) {
    				// 如果有右子树，找到右子树的最左节点
    				if (_node->_right) {
    					_node = _node->_right;
    					while (_node->_left) {
    						_node = _node->_left;
    					}
    				} else {
    					// 否则向上找到第一个父节点，当前节点在其左子树中
    					typename rbt::Node* parent = _node->_parent;
    					while (parent && _node == parent->_right) {
    						_node = parent;
    						parent = parent->_parent;
    					}
    					// 检查是否到达了 header（即遍历完成）
    					if (parent == _tree->getHeader()) {
    						_node = nullptr;
    					} else {
    						_node = parent;
    					}
    				}
    			}
    			return *this;
    		}
    		iterator operator++(int) {
    			iterator temp = *this;
    			++(*this);
    			return temp;
    		}
    		std::pair<K, V>& operator*() const {
    			return _node->_val;
    		}
    		std::pair<K, V>* operator->() const {
    			return &(_node->_val);
    		}
    		bool operator==(const iterator& other) const {
    			return _node == other._node && _map == other._map;
    		}
    		bool operator!=(const iterator& other) const {
    			return !(*this == other);
    		}
    	};
    	friend class iterator;
    private:
    	bool insert(const std::pair<K, V>& kv){
    		return _rbt.insert(kv);
    	}
    public:
        Map_CTL() = default;
        ~Map_CTL() {
        	clear();
        }
        void put(const K& key, const V& value) {
        	insert(std::pair<K, V>(key,value));
        }
        V* get(const K& key) {
        	typename rbt::Node* node = _rbt.findNode(key);
        	if (node) {
        		return &(node->_val.second);
        	}
        	return nullptr;
        }
        void remove(const K& key) {
        	_rbt.erase(key);
        }
    	void clear() {
        	_rbt.clear();
        }
    	iterator begin() {
        	return iterator(_rbt.leftMostPublic(), &_rbt);
        }
    	iterator end() {
        	return iterator(nullptr, &_rbt);
        }
    };
}

#endif
