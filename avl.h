#ifndef __AVL_H__
#define __AVL_H__

#include "binarytree.h"

template <typename Traits>
class CAVLNode : public CBinaryTreeNode<value_type>{
public:
  using value_type = typename Traits::T;
  using Node       = CBinaryTreeNode<value_type>;
protected:
    int     m_balanceFactor = 0; // Balance factor for AVL tree
public:
};

template <typename _T>
struct AVLAscTraits{
    using  value_type = _T;
    using  Node       = CAVLNode<T>;
    using  CompareFn  = less<T>;
};

template <typename _T>
struct AVLDescTraits{
    using  value_type = _T;
    using  Node       = CAVLNode<T>;
    using  CompareFn  = greater<T>;
};

template <typename Traits>
class CAVLTree : public CBinaryTree<Traits> {
public:
    using Base       = CBinaryTree<Traits>;
    using Node       = typename Traits::Node;
    using value_type = typename Traits::value_type;  
    using CompareFn  = typename Traits::CompareFn;
    using Container  = CAVLTree<Traits>;
    using iterator   = binary_tree_iterator<Container>;

protected:
    // Additional members for AVL tree balancing can be added here
    Node *internal_insert(value_type &elem, Ref ref,
                          Node* pParent, Node*& rpOrigin) override
    {
        Node* newNode = Base::internal_insert(elem, ref, pParent, rpOrigin);
        // TODO: Verificar balance y realizar rotaciones si es necesario
        
        std::function<int(Node*)> height = [&](Node* n)->int{
            if (!n) return 0;
            int hl = height(n->getChild(0));
            int hr = height(n->getChild(1));
            return 1 + (hl > hr ? hl : hr);
        };

        // Helper: compute balance factor as height(right) - height(left)
        auto recompute_balance = [&](Node* n)->int{
            if (!n) return 0;
            return height(n->getChild(1)) - height(n->getChild(0));
        };

        // Rotations (update parent/child pointers). Return new root of the rotated subtree
        std::function<Node*(Node*)> rotateLeft = [&](Node* x)->Node*{
            if (!x) return x;
            Node* y = x->getChild(1);
            if (!y) return x;

            Node* yLeft = y->getChild(0);
            x->getChildRef(1) = yLeft;
            if (yLeft) yLeft->m_pParent = x;
            Node* parent = x->getParent();

            y->getChildRef(0) = x;
            x->m_pParent = y;

            y->m_pParent = parent;
            if (parent) {
                if (parent->getChild(0) == x) parent->getChildRef(0) = y;
                else parent->getChildRef(1) = y;
            } else {
                // x was root
                this->m_pRoot = y;
            }

            x->m_balanceFactor = recompute_balance(x);
            y->m_balanceFactor = recompute_balance(y);
            return y;
        };

        std::function<Node*(Node*)> rotateRight = [&](Node* x)->Node*{
            if (!x) return x;
            Node* y = x->getChild(0);
            if (!y) return x;
            Node* yRight = y->getChild(1);
            x->getChildRef(0) = yRight;
            if (yRight) yRight->m_pParent = x;
            Node* parent = x->getParent();
            y->getChildRef(1) = x;
            x->m_pParent = y;
            y->m_pParent = parent;
            if (parent) {
                if (parent->getChild(0) == x) parent->getChildRef(0) = y;
                else parent->getChildRef(1) = y;
            } else {
                this->m_pRoot = y;
            }
            x->m_balanceFactor = recompute_balance(x);
            y->m_balanceFactor = recompute_balance(y);
            return y;
        };

        if (newNode) {
            Node* p = newNode->getParent();
            while (p) {
                // recompute balance for p
                p->m_balanceFactor = recompute_balance(p);
                if (p->m_balanceFactor > 1) {
                    Node* r = p->getChild(1);
                    if (r && recompute_balance(r) < 0) {
                        // Right-Left case
                        rotateRight(r);
                    }
                    rotateLeft(p);
                    break; 
                } else if (p->m_balanceFactor < -1) {
                    // Left heavy
                    Node* l = p->getChild(0);
                    if (l && recompute_balance(l) > 0) {
                        // Left-Right case
                        rotateLeft(l);
                    }
                    rotateRight(p);
                    break;
                }
                // If balance becomes 0, the height didn't grow further
                if (p->m_balanceFactor == 0) break;
                p = p->getParent();
            }
        }
        
        return newNode;
    }
public:
    CAVLTree() : Base() {} // Empty tree

};

#endif // __AVL_H__