#ifndef __BTREE_H__
#define __BTREE_H__

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include "btreepage.h"
#define DEFAULT_BTREE_ORDER 3

const size_t MaxHeight = 5; 

template <typename _keyType, typename _ObjIDType, typename _CompareFunction>
struct BTreeTrait
{
    using keyType = _keyType; ///< Tipo de las claves.
    using ObjIDType = _ObjIDType; ///< Tipo de los identificadores de objeto.
    
    // TODO: agregar función de comparación
    using CompareFunction = _CompareFunction;
};


template <typename Trait>
class BTree
{
    typedef typename Trait::keyType keyType; ///< Tipo de las claves del árbol.
    typedef typename Trait::ObjIDType ObjIDType; ///< Tipo de los identificadores de objeto.
    typedef typename Trait::CompareFunction CompareFunction; ///< Función de comparación.
    typedef CBTreePage<Trait> BTNode; ///< Nodo del árbol B (CBTreePage).
    
public:
    // typedef CBTreePage<Trait> BTNode; ///< Nodo del árbol B (CBTreePage) solo para el testmove
    typedef typename BTNode::ObjectInfo ObjectInfo; ///< Información del objeto almacenado en el nodo.

    BTree(size_t order = DEFAULT_BTREE_ORDER, bool unique = true)
        : m_Order(order), m_Root(2 * order + 1, unique), m_Unique(unique), m_NumKeys(0)
    {
        m_Root.SetMaxKeysForChilds(order);
        m_Height = 1;
    }

    ~BTree() {}

    //move constructor en btree
    BTree(BTree<Trait> &&other);

    // move asignment operator en btree
    BTree &operator=(BTree<Trait> &&other);


    //Definir iteradores forward y backward
    class Iterator
    {
        
    };

    bool Insert(const keyType key, const ObjIDType ObjID);

    bool Remove(const keyType key, const ObjIDType ObjID);

    ObjIDType Search(const keyType key)
    {
        // std::lock_guard<std::shared_mutex> lock(m_mutex);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        ObjIDType ObjID = ObjIDType();
        
        if (m_Root.Search(key, ObjID))
            return ObjID;
        return -1;

    }

    size_t size() { 
        // std::lock_guard<std::shared_mutex> lock(m_mutex);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_NumKeys; 
    }

    size_t height() { 
        // std::lock_guard<std::shared_mutex> lock(m_mutex); 
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_Height; 
    }

    size_t GetOrder() { 
        // std::lock_guard<std::shared_mutex> lock(m_mutex); 
        std::shared_lock<std::shared_mutex> lock(m_mutex); 
        return m_Order; 
    }

    void Print(std::ostream &os) { 
        // std::lock_guard<std::shared_mutex> lock(m_mutex); 
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        m_Root.Print(os); 
    }

    std::ostream& Write(std::ostream &os);

    std::istream& Read(std::istream &is);

    std::ostream& WriteBinaryTreeFormat(std::ostream& os);

    std::istream& ReadBinaryTreeFormat(std::istream& is);


    template <typename Function>
    void ForEach(Function fn);
    
    template <typename Function>
    ObjectInfo* FirstThat(Function fn);

protected:
    BTNode m_Root; ///< Nodo raíz del árbol B.
    size_t m_Height; ///< Altura del árbol.
    size_t m_Order; ///< Orden del árbol (máximo número de hijos por nodo).
    size_t m_NumKeys; ///< Número de claves en el árbol.
    bool m_Unique; ///< Si es `true`, los elementos deben ser únicos.

    std::shared_mutex m_mutex;
};


//move constructor en btree
template <typename Trait>
BTree<Trait>::BTree(BTree&& other){
    // std::lock_guard<std::shared_mutex> lock(other.m_mutex);
    // std::unique_lock<std::shared_mutex> lock(other.m_mutex);
    std::scoped_lock locks(m_mutex, other.m_mutex);
    m_Root      = std::move(other.m_Root);    
    m_Height    = std::exchange(other.m_Height, 1);
    m_Order     = std::exchange(other.m_Order, DEFAULT_BTREE_ORDER);
    m_NumKeys   = std::exchange(other.m_NumKeys, 0);
    m_Unique    = std::exchange(other.m_Unique, true);
}

template <typename Trait>
BTree<Trait>& BTree<Trait>::operator=(BTree &&other){
    if(this != &other){
        std::scoped_lock locks(m_mutex, other.m_mutex);  //es para evitar el deadlock
        m_Root      = std::move(other.m_Root);        
        m_Height    = std::exchange(other.m_Height, 1);
        m_Order     = std::exchange(other.m_Order, DEFAULT_BTREE_ORDER);
        m_NumKeys   = std::exchange(other.m_NumKeys, 0);
        m_Unique    = std::exchange(other.m_Unique, true);
    }
    
    return *this;
}


template <typename Trait>
bool BTree<Trait>::Insert(const keyType key, const ObjIDType ObjID)
{
    // std::lock_guard<std::shared_mutex> lock(m_mutex);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    bt_ErrorCode error = m_Root.Insert(key, ObjID);
    if (error == bt_duplicate)
        return false;
    m_NumKeys++;
    if (error == bt_overflow) {
        m_Root.SplitRoot();
        m_Height++;
    }
    return true;
}

template <typename Trait>
bool BTree<Trait>::Remove(const keyType key, const ObjIDType ObjID)
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::unique_lock<std::shared_mutex> lock(m_mutex);
    bt_ErrorCode error = m_Root.Remove(key, ObjID);
    if (error == bt_duplicate || error == bt_nofound)
        return false;
    m_NumKeys--;
    if (error == bt_rootmerged)
        m_Height--;
    return true;
}

//implementamos foreach
template <typename Trait>
template <typename Function>
void BTree<Trait>::ForEach(Function fn)
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::shared_lock<std::shared_mutex> lock(m_mutex);
    m_Root.ForEach(fn, 0);
}

//implementamos FirstThat
template <typename Trait>
template <typename Function>
typename BTree<Trait>::ObjectInfo* BTree<Trait>::FirstThat(Function fn)
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_Root.FirstThat(fn, 0);
}

 template <typename Trait>
std::ostream& BTree<Trait>::Write(std::ostream &os) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::unique_lock<std::shared_mutex> lock(m_mutex);
    // Cabecera 
    os << "BTree " << m_Order << " " << m_Height << " " << m_NumKeys << " " << m_Unique << "\n";
    
    // Escribimos la raíz
    m_Root.Write(os);
    
    return os;
}

template <typename Trait>
std::istream& BTree<Trait>::Read(std::istream &is) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::string tag;
    is >> tag; // leemos la cabecera 
    
    if(tag == "BTree") {
        is >> m_Order >> m_Height >> m_NumKeys >> m_Unique;
        
        // Leemos la raíz
        m_Root.Read(is);
    }
    
    return is;
}

template <typename Trait>
std::ostream& BTree<Trait>::WriteBinaryTreeFormat(std::ostream& os)
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    // std::unique_lock<std::shared_mutex> lock(m_mutex);
    os << "BTreeSimple " << m_NumKeys << " elements: ";
    ForEach([&os](auto& info, size_t level) {
        os << info.key << " ";
    });
    return os;
}

template <typename Trait>
std::istream& BTree<Trait>::ReadBinaryTreeFormat(std::istream& is)
{
    // std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string line;
    std::getline(is, line); // Leemos la primera línea

    m_Root.Reset(); // Reseteamos el árbol.
    m_NumKeys = 0;
    m_Height = 1;

    // Convertimos cada carácter de la línea a una clave numérica y la insertamos en el árbol
    for (char c : line) {
        if (std::isdigit(c)) {
            int key = c - '0';  // Convertir char a int
            Insert(key, key);
        }
    }

    return is;
}

template <typename Trait>
std::ostream& operator<<(std::ostream& os, BTree<Trait>& tree)
{
    tree.Print(os);  // Llama al método Print de la clase BTree para generar la salida
    return os;
}

#endif // __BTREE_H__
