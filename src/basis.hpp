#ifndef BASIS_AST
#define BASIS_AST

#include <memory>

namespace fpc
{
    class BasicNode
    {
    public:
        BasicNode() {}
        virtual ~BasicNode() = default;
    };

    template <typename T>
    class ListNode: public BasicNode
    {
    private:
        std::list<std::shared_ptr<T>> children;
    public:
        ListNode() {}
        ListNode(const std::shared_ptr<T> &val) { children.push_back(val); }
        ~ListNode() {}

        std::list<std::shared_ptr<T>> &getChildren() { return children; }

        void append(const std::shared_ptr<T> &val) { children.push_back(val); }
       
    };

    class ExprNode: public BasicNode
    {
    public:
        ExprNode() {}
        ~ExprNode() {}
    };
    
    class StmtNode: public BasicNode
    {
    public:
        StmtNode() {}
        ~StmtNode() {}
    };
    
    //make new node
    template<typename T, typename... Args>
    std::shared_ptr<T> make_new_node(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    //TODO:
    template<typename T, typename U>
    inline typename std::enable_if<std::is_base_of<BasicNode, U>::value && std::is_base_of<BasicNode, T>::value, bool>::type 
    is_ptr_of(const std::shared_ptr<U> &ptr)
    {
        return dynamic_cast<T *>(ptr.get()) != nullptr;
    }

    template<typename T, typename U>
    inline typename std::enable_if<std::is_base_of<BasicNode, U>::value && std::is_base_of<BasicNode, T>::value, std::shared_ptr<T>>::type
    cast_node(const std::shared_ptr<U> &ptr)
    {
        return std::dynamic_pointer_cast<T>(ptr);
    }
    
} 

#endif