#ifndef CTL_ARRAY_CTL_H
#define CTL_ARRAY_CTL_H

namespace CTL {
    template<typename V>
    class Array_CTL {
    public:
    private:
        struct BinaryTree {
            size_t Key = 0;
            V* value = nullptr;
            BinaryTree* left = nullptr;
            BinaryTree* right = nullptr;
        };
        BinaryTree* root = nullptr;
        size_t _size_m = 0;
    public:
        Array_CTL() {
            delete root;
            root = new BinaryTree();
        }
        ~Array_CTL() {
            clear(this->root);
        }
        void add(const V value) {
            insert(_size_m, value);
        }
        void clear() {
            clear(this->root);
        }
        V get(const size_t index) {
            return *find(index,this->root);
        }
        V* getPtr(const size_t index) {
            return find(index,this->root);
        }
    private:
        void clear(BinaryTree* node) {
            if (node) {
                clear(node->left);
                clear(node->right);
                delete node->value;
                node->value = nullptr;
                delete node;
                node = nullptr;
                _size_m--;
            }
        }
        V* find(int key, const BinaryTree* node) {
            V* value = nullptr;
            if (node) {
                if (node->Key == key) {
                    return node->value;
                }
                else if (node->Key < key) {
                    if (node->right) {
                        return find(key,node->right);
                    }
                }
                else if (node->Key > key) {
                    if (node->left) {
                        return find(key,node->left);
                    }
                }
            }
            return value;
        }
        void insert(size_t key,V value) {
            V* newValue = new V(value);

            if (!this->root) {
                this->root = new BinaryTree();
                this->root->Key = key;
                this->root->value = newValue;
                _size_m++;
                return;
            }

            BinaryTree* current = this->root;
            while (current) {
                if (key == current->Key) {
                    if (current->value) {
                        delete current->value;
                    }
                    else {
                        _size_m++;
                    }
                    current->value = newValue;
                    return;
                }
                else if (key < current->Key) {
                    if (!current->left) {
                        current->left = new BinaryTree();
                        current->left->Key = key;
                        current->left->value = newValue;
                        _size_m++;
                        return;
                    }
                    current = current->left;
                }
                else {
                    if (!current->right) {
                        current->right = new BinaryTree();
                        current->right->Key = key;
                        current->right->value = newValue;
                        _size_m++;
                        return;
                    }
                    current = current->right;
                }
            }
        }
    };
}

#endif
