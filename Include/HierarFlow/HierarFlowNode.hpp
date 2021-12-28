#pragma once

#include "Utils/Common.hpp"
#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"
#include "Logger/Logger.hpp"

template<class I, class O>
class HierarFlowRoutine;

template<class I>
class HierarFlowCallee;

template<class O>
class HierarFlowCaller;

template<class... OArgs>
class HierarFlowCaller<void(OArgs...)>;

template<class I, class O>
class HierarFlowNodeImpl;

template<class I, class O>
class HierarFlowRoutineWrapper;

template<class I, class O>
class HierarFlowNode;

template<class A, class B, class C, class D>
class HierarFlowPath;

template<class A, class B, class C>
class HierarFlowChildren;

// HierarFlowCallee represents objects that are called by their predecessors
// with the arguments of type "IArgs..."

template<class IReturn, class... IArgs>
class HierarFlowCallee<IReturn(IArgs...)> {
    using I = IReturn(IArgs...);

public:
    HierarFlowCallee() : parent(nullptr) {} 

    HierarFlowCallee(HierarFlowCaller<I> *parent) : parent(parent) {}

    virtual ~HierarFlowCallee() {}

    virtual NullableRef<HierarFlowCallee<I>>
        operator()(IArgs... args) = 0;

    HierarFlowCallee<I>& operator=(HierarFlowCallee<I>&& orig) {
        next_node.swap(orig.next_node);
        return *this;
    }

    HierarFlowCallee<I>& SetParentAndGetLastChild(HierarFlowCaller<I>* caller) {
        if (parent) {
            ERROR("Node's parent can be set only once. " 
                  "You are trying to use the same node mutiple times "
                  "in difference places.");
        }

        parent = caller;
        if (next_node) return next_node->SetParentAndGetLastChild(caller);
        return *this;
    }

public:
    // FIXME: I couldn't make compilation pass using NullableRef<HierarFlowCaller<I>>
    // But probably it's possible actually.
    HierarFlowCaller<I> *parent;
    std::shared_ptr<HierarFlowCallee<I>> next_node;
//protected:
};

// HierarFlowCaller represents objects that call their successors
// with the arguments of type "OArgs..."

template<class OReturn, class... OArgs>
class HierarFlowCaller<OReturn(OArgs...)> {
    using O = OReturn(OArgs...);
    friend HierarFlowCallee<O>;

public:
    virtual ~HierarFlowCaller() {}

    OReturn& GetResponseValue() {
        return resp_val;
    }
    
public:
//protected:
    void SetLastSucc(HierarFlowCallee<O> &last_succ) {
        this->last_succ_ref = std::ref(last_succ);
    }

    // the following is illegal if OReturn = void, so we specialize that case below
    OReturn resp_val; 

    NullableRef<HierarFlowCallee<O>> last_succ_ref;
};

template<class... OArgs>
class HierarFlowCaller<void(OArgs...)> {
    using O = void(OArgs...);
    friend HierarFlowCallee<O>;

public:
    virtual ~HierarFlowCaller() {}

public:
//protected:
    void SetLastSucc(HierarFlowCallee<O> &last_succ) {
        this->last_succ_ref = std::ref(last_succ);
    }

    NullableRef<HierarFlowCallee<O>> last_succ_ref;
};

// Why do we need HierarFlowNodeImpl?: see the comment of HierarFlowNode.
template<class IReturn, class... IArgs, class OReturn, class... OArgs>
class HierarFlowNodeImpl<IReturn(IArgs...), OReturn(OArgs...)> 
    : public HierarFlowCallee<IReturn(IArgs...)>, 
      public HierarFlowCaller<OReturn(OArgs...)> 
{
    using I = IReturn(IArgs...);
    using O = OReturn(OArgs...);

public:
    std::shared_ptr<HierarFlowRoutine<I, O>> ShareRoutine(void) {
        return routine;
    }

    template<class O2>
    void AddSuccessor(std::shared_ptr<HierarFlowNodeImpl<O, O2>> succ_node) {
        if (!first_succ_node) {
            assert(!this->last_succ_ref);
            first_succ_node = succ_node;
        } else {
            auto &last_succ = this->last_succ_ref.value().get();
            last_succ.next_node = succ_node;
        }
        this->SetLastSucc(succ_node->SetParentAndGetLastChild(this));
    }

    template<class O2>
    void SetNext(std::shared_ptr<HierarFlowNodeImpl<I, O2>> node) {
        if (this->next_node) {
            ERROR("Node's next can be set only once.\n"
                  "You are trying to use the same node mutilple times "
                  "in difference places.\n"
                  "If you want to do it, use HardLink()." );
        }

        if (this->parent) {
            auto& last_succ = node->SetParentAndGetLastChild(this->parent);
            this->parent->SetLastSucc(last_succ);
        }

        this->next_node = node;
    }


// FIXME: 設計し直し
public:
    HierarFlowNodeImpl() {}

    HierarFlowNodeImpl(HierarFlowNodeImpl<I, O>&& orig)
        : HierarFlowCallee<I>(orig),
          HierarFlowCaller<O>(orig),
          routine(std::move(orig.routine)),
          first_succ_node(std::move(orig.first_succ_node)) {}

    HierarFlowNodeImpl<I, O>& operator=(HierarFlowNodeImpl<I, O> &&orig) {
        HierarFlowCallee<I>::operator=(std::move(orig));
        HierarFlowCaller<O>::operator=(std::move(orig));

        //parent = orig.parent;
        routine.swap(orig.routine);
        first_succ_node.swap(orig.first_succ_node);
        return *this;
    }

//private:
    HierarFlowNodeImpl(
        std::shared_ptr<HierarFlowRoutine<I, O>> routine
    ) : HierarFlowCallee<I>(),
        HierarFlowCaller<O>(),
        routine(routine) {}

    HierarFlowNodeImpl(
        HierarFlowCaller<I>& parent,
        std::shared_ptr<HierarFlowRoutine<I, O>> routine
    ) : HierarFlowCallee<I>(&parent), 
        HierarFlowCaller<O>(),
        routine(routine) {}

    NullableRef<HierarFlowCallee<I>> operator()(IArgs... args) {
        // FIXME: currently we don't save the previous values 
        // of first_succ_ref, ..., parent_resp_val_ref.
        // This doesn't matter unless 
        // there are two nodes that share the same routine, 
        // and one of which is an offspring of the other.

        if (first_succ_node) {
            routine->SetFirstSuccRef(std::ref(*first_succ_node));
        } else {
            routine->SetFirstSuccRef(std::nullopt);
        }

        if (this->next_node) {
            routine->SetNextRef(std::ref(*this->next_node));
        } else {
            routine->SetNextRef(std::nullopt);
        }

        if constexpr ( !std::is_same_v<OReturn, void> ) {
            // Initialize OReturn. This means, OReturn must be a type which is movable and which has the default constructor
            this->resp_val = OReturn(); 
            routine->SetOwnResponseValueRef(this->resp_val);
        }

        if constexpr ( !std::is_same_v<IReturn, void> ) {
            routine->SetParentResponseValueRef(this->parent->resp_val);
        }

        return (*routine)(args...);
    }

    std::shared_ptr<HierarFlowRoutine<I, O>> routine; 
    std::shared_ptr<HierarFlowCallee<O>> first_succ_node;
};
