#pragma once

#include <memory>

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"

// 多分、後で個々のクラスごとにヘッダ分ける

// 登場人物
// node, path, child

// operators to be defined and its current status
/* << */
// node << node done
// node << path done
// node << child done

// path << node done
// path << path done
// path << child done

/* || */
// node || node done
// node || child done
// node || path done

// child || node done
// child || path done
// child || child done

// path || node done 
// path || path done
// path || child done

// node [node]
// node [path]
// node [child]

// path [node]
// path [path]
// path [child]

template<class I, class O>
class HierarFlowRoutine;

template<class I>
class HierarFlowCallee;

template<class O>
class HierarFlowCaller;

template<class I, class O>
class HierarFlowNodeImpl;

template<class HeadI, class HeadO, class TailI, class TailO>
class HierarFlowPath;

template<class I, class HeadO, class TailO>
class HierarFlowChildren;

template<class I, class O>
class HierarFlowNode;

// The wrapper of HierarFlowNodeImpl:
// In order to support operators on HierarFlowNodeImpl, 
// HierarFlowNodeImpl should be always handled via std::shared_ptr.
// This means we should not expose HierarFlowNodeImpl itself to users
// because it may result in using HierarFlowNodeImpl without std::shared_ptr.
// Therefore this is the class actually the instance of which users can create.
template<class IReturn, class... IArgs, class O> 
class HierarFlowNode<IReturn(IArgs...), O> {
    using I = IReturn(IArgs...);

    template<class A, class B>
    friend class HierarFlowNode;

    template<class A, class B, class C, class D>
    friend class HierarFlowPath;

    template<class A, class B, class C>
    friend class HierarFlowChildren;

public:
    HierarFlowNode() {}
 
    HierarFlowNode(std::shared_ptr<HierarFlowRoutine<I, O>> routine) 
        : impl(std::make_shared<HierarFlowNodeImpl<I, O>>(routine)) {}

    HierarFlowNode(const HierarFlowNode<I, O>& src) {
        impl = src.impl;
    }

    HierarFlowNode(HierarFlowNode<I, O>&& src) {
        impl = std::move(src.impl);
    }

    HierarFlowNode<I, O>& operator=(const HierarFlowNode<I, O>& src) {
        impl = src.impl;
        return *this;
    }

    HierarFlowNode<I, O>& operator=(HierarFlowNode<I, O>&& orig) {
        impl = std::move(orig.impl);
        return *this;
    }

    HierarFlowNodeImpl<I, O>& operator*() const {
        return *impl;
    }

    HierarFlowNodeImpl<I, O>* operator->() const {
        return impl.get();
    }

    HierarFlowNode<I, O> HardLink() {
        return HierarFlowNode<I, O>(impl->ShareRoutine());
    }

    template<class O2>
    HierarFlowPath<I, O, O, O2> operator<<(HierarFlowNode<O, O2> succ) {
        impl->AddSuccessor(succ.impl);
        return HierarFlowPath<I, O, O, O2>(impl, *succ.impl);
    }

    template<class T, class TailI, class TailO>
    HierarFlowPath<I, O, TailI, TailO> operator<<(HierarFlowPath<O, T, TailI, TailO> path) {
        impl->AddSuccessor(path.head);
        return HierarFlowPath<I, O, TailI, TailO>(impl, path.tail);
    }

    template<class ChildHeadO, class ChildTailO>
    HierarFlowPath<I, O, O, ChildHeadO> operator<<(HierarFlowChildren<O, ChildHeadO, ChildTailO> children) {
        impl->AddSuccessor(children.first_child);
        return HierarFlowPath<I, O, O, ChildHeadO>(impl, *children.first_child);
    }

    template<class O2>
    HierarFlowChildren<I, O, O2> operator||(HierarFlowNode<I, O2> brother) {
        impl->SetNext(brother.impl);
        return HierarFlowChildren<I, O, O2>(impl, *brother.impl);
    }

    template<class O2, class O3>
    HierarFlowChildren<I, O, O3> operator||(HierarFlowChildren<I, O2, O3> children) {
        impl->SetNext(children.first_child);
        return HierarFlowChildren<I, O, O3>(impl, children.last_child);
    }

    template<class HeadO, class TailI, class TailO>
    HierarFlowChildren<I, O, HeadO> operator||(HierarFlowPath<I, HeadO, TailI, TailO> path) {
        impl->SetNext(path.head);
        return HierarFlowChildren<I, O, HeadO>(impl, path.head);
    }

    // We define operator() only if IReturn == void, in order to avoid crashes(SFINAE).
    // For details, see the comment of HeadWrapperRoutine.
    template<class IReturn_ = IReturn>
    auto operator()(IArgs... args)
        -> std::enable_if_t< std::is_void_v< IReturn_ >, void > {

        (*impl)(args...);
    }

    // If IReturn == void, then simply emit a compilation error with static_assert.
    template<class IReturn_ = IReturn>
    auto operator()(IArgs... args)
        -> std::enable_if_t< !std::is_void_v< IReturn_ >, void > {

        static_assert( std::is_void_v< IReturn_ >, "You cannot use operator() when IReturn != void. Use WrapToMakeHeadNode." );
    }

private:
    std::shared_ptr<HierarFlowNodeImpl<I, O>> impl;
};

// The class which describes a "chain" of nodes like a << b << c.
template<class HeadI, class HeadO, class TailI, class TailO>
class HierarFlowPath {

    template<class A, class B>
    friend class HierarFlowNode;

    template<class A, class B, class C, class D>
    friend class HierarFlowPath;

    template<class A, class B, class C>
    friend class HierarFlowChildren;

public:
    HierarFlowPath(
        std::shared_ptr<HierarFlowNodeImpl<HeadI, HeadO>> head, 
        HierarFlowNodeImpl<TailI, TailO> &tail
    ) : head(head),
        tail(tail) {}

    template<class NewTailO>
    HierarFlowPath<HeadI, HeadO, TailO, NewTailO> operator<<(
        HierarFlowNode<TailO, NewTailO> new_tail
    ) {
        tail.AddSuccessor(new_tail.impl);
        return HierarFlowPath<HeadI, HeadO, TailO, NewTailO>(head, *new_tail.impl);
    }

    template<class T, class NewTailI, class NewTailO>
    HierarFlowPath<HeadI, HeadO, NewTailI, NewTailO> operator<<(
        HierarFlowPath<TailO, T, NewTailI, NewTailO> path
    ) {
        tail.AddSuccessor(path.head);
        return HierarFlowPath<HeadI, HeadO, NewTailI, NewTailO>(head, path.tail);
    }

    template<class ChildHeadO, class ChildTailO>
    HierarFlowPath<HeadI, HeadO, TailO, ChildHeadO> operator<<(
        HierarFlowChildren<TailO, ChildHeadO, ChildTailO> children
    ) {
        tail.AddSuccessor(children.first_child);
        return HierarFlowPath<HeadI, HeadO, TailO, ChildHeadO>(head, *children.first_child);
    }

    template<class O>
    HierarFlowChildren<HeadI, HeadO, O> operator||(HierarFlowNode<HeadI, O> new_last) {
        head->SetNext(new_last.impl);
        return HierarFlowChildren<HeadI, HeadO, O>(head, *new_last.impl);
    }

    template<class A, class B, class C>
    HierarFlowChildren<HeadI, HeadO, A> operator||(HierarFlowPath<HeadI, A, B, C> path) {
        head->SetNext(path.head);
        return HierarFlowChildren<HeadI, HeadO, A>(head, *path.head);
    }

    template<class ChildHeadO, class ChildTailO>
    HierarFlowChildren<HeadI, HeadO, ChildHeadO> operator||(
        HierarFlowChildren<HeadI, ChildHeadO, ChildTailO> children
    ) {
        head->SetNext(children.first_child);
        return HierarFlowChildren<HeadI, HeadO, ChildHeadO>(head, children.last_child);
    }

private:
    std::shared_ptr<HierarFlowNodeImpl<HeadI, HeadO>> head;
    HierarFlowNodeImpl<TailI, TailO> &tail;
};

// The class which describes a sequence of child nodes like a || b || c
template<class I, class HeadO, class TailO>
class HierarFlowChildren {

    template<class A, class B>
    friend class HierarFlowNode;

    template<class A, class B, class C, class D>
    friend class HierarFlowPath;

    template<class A, class B, class C>
    friend class HierarFlowChildren;

public:
    HierarFlowChildren(std::shared_ptr<HierarFlowNodeImpl<I, HeadO>> first_child, HierarFlowNodeImpl<I, TailO> &last_child)
        : first_child(first_child),
          last_child(last_child) {}

    template<class O>
    HierarFlowChildren<I, HeadO, O> operator||(HierarFlowNode<I, O> new_last) {
        last_child.SetNext(new_last.impl);
        return HierarFlowChildren<I, HeadO, O>(first_child, *new_last.impl);
    }

    template<class A, class B, class C> 
    HierarFlowChildren<I, HeadO, A> operator||(HierarFlowPath<I, A, B, C> path) {
        last_child.SetNext(path.head);
        return HierarFlowChildren<I, HeadO, A>(first_child, *path.head);
    }

    template<class O, class O2>
    HierarFlowChildren<I, HeadO, O2> operator||(HierarFlowChildren<I, O, O2> children) {
        last_child.SetNext(children.first_child);
        return HierarFlowChildren<I, HeadO, O2>(first_child, children.last_child);
    }

private:
    std::shared_ptr<HierarFlowNodeImpl<I, HeadO>> first_child;
    HierarFlowNodeImpl<I, TailO> &last_child;
};

namespace pipeline {

template<class RoutineDerived, class... Args>
HierarFlowNode< typename RoutineDerived::InputType, typename RoutineDerived::OutputType> 
    CreateNode(Args&&... args) 
{
    using I = typename RoutineDerived::InputType;
    using O = typename RoutineDerived::OutputType;
    
    return HierarFlowNode<I, O> (
        std::shared_ptr<HierarFlowRoutine<I, O>>(new RoutineDerived(args...))
    );
}

// Sometimes, users may want to use, as the root node, routines whose IReturn is not void.
// In that case, calling SetReponseValue in the root routine causes a crash 
// because it expects the reference of parent's resp_val(which is actually null).
// Therefore, we provide the way of creating a dummy root which just calls the successors with provided arguments.
template<class IReturn, class... IArgs>
class HeadWrapperRoutine
    : public HierarFlowRoutine<void(IArgs...), IReturn(IArgs...)> {

public:
      HeadWrapperRoutine(void) {}

      NullableRef<HierarFlowCallee<void(IArgs...)>> operator()(IArgs... args) {
          this->CallSuccessors(args...);
          return this->GoToParent();
      }
};

template<class IReturn, class... IArgs, class O>
HierarFlowNode<void(IArgs...), IReturn(IArgs...)> WrapToMakeHeadNode(
    HierarFlowNode<IReturn(IArgs...), O> node
) {
    auto head = CreateNode<HeadWrapperRoutine<IReturn, IArgs...>>();
    head << node;
    return head;
}

} // namespace pipeline
