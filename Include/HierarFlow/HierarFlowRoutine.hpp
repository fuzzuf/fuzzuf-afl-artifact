#pragma once

#include <type_traits>

#include "Utils/Common.hpp"
#include "HierarFlow/HierarFlowNode.hpp"

template<class I, class O>
class HierarFlowNodeImpl;

template<class I>
class HierarFlowCallee;

template<class I, class O>
struct HierarFlowRoutine;

// ResponseValue: child nodes can send a value to parent node
// this is something like return value
// the below struct is used for storing its reference
template<class T>
struct ResponseValueRef {
    NullableRef<T> ref;
};

// if the type of ResponseValue is set to void, then we don't need anything
// but still we must define the below because NullableRef<void> is illegal
template<>
struct ResponseValueRef<void> {};

template<class IReturn, class... IArgs, class OReturn, class... OArgs>
struct HierarFlowRoutine<IReturn(IArgs...), OReturn(OArgs...)> {
    using I = IReturn(IArgs...);
    using O = OReturn(OArgs...);
    friend HierarFlowNodeImpl<I, O>;

public:
    using InputType = I;
    using OutputType = O;

    virtual ~HierarFlowRoutine() {}

protected:

    // Only if IReturn is not void, we can get a response value.
    // Define the getter using SFINAE
    template<class IReturn_ = IReturn>
    auto GetResponseValue(void)
        -> std::enable_if_t< !std::is_void_v< IReturn_ >, 
                             typename std::add_lvalue_reference<IReturn>::type > {

        // Because IReturn_ is not identical to IReturn, 
        // one can wrongly call this function even if IReturn == void
        static_assert( !std::is_void_v< IReturn >, 
          "You cannot use GetResponseValue when IReturn == void");

        return parent_response_ref.ref.value().get();
    }

    // Only if IReturn is not void, we can set a response value.
    // Define the setter using SFINAE
    template<class IReturn_ = IReturn>
    auto SetResponseValue(IReturn_ val) 
        -> std::enable_if_t< !std::is_void_v< IReturn_ >, void > {

        // Because IReturn_ is not identical to IReturn, 
        // one can wrongly call this function even if IReturn == void
        static_assert( !std::is_void_v< IReturn >, 
          "You cannot use SetResponseValue when IReturn == void");

        parent_response_ref.ref.value().get() = val;
    }

    NullableRef<HierarFlowCallee<I>> GoToParent(void) {
        return std::nullopt;
    }

    NullableRef<HierarFlowCallee<I>> GoToDefaultNext(void) {
        return next_node_ref;
    }

    OReturn CallSuccessors(OArgs... args) {
        auto succ_ref = first_succ_ref;
        while (succ_ref) {
            auto& succ = succ_ref.value().get();
            auto next_succ_ref = succ(args...);
            succ_ref.swap(next_succ_ref);
        } 
        
        if constexpr ( std::is_same_v<OReturn, void> ) {
            return;
        } else {
            return own_response_ref.ref.value().get();
        } 
    }

public:
//private:
    virtual NullableRef<HierarFlowCallee<I>> 
        operator()(IArgs... args) = 0;

    void SetFirstSuccRef(NullableRef<HierarFlowCallee<O>> node_ref) {
        first_succ_ref = node_ref;
    }

    void SetNextRef(NullableRef<HierarFlowCallee<I>> node_ref) {
        next_node_ref = node_ref;
    }

    // Only if OReturn is not void, we need to set own_response_ref.
    // Define the setter using SFINAE
    template<class OReturn_ = OReturn>
    auto SetOwnResponseValueRef(OReturn_& own_resp_val)
        -> std::enable_if_t< !std::is_void_v< OReturn_ >, void > {
        own_response_ref.ref = std::ref(own_resp_val);
    }

    // Only if OReturn is not void, we need to set parent_response_ref.
    // Define the setter using SFINAE
    template<class IReturn_ = IReturn>
    auto SetParentResponseValueRef(IReturn_& parent_resp_val) 
        -> std::enable_if_t< !std::is_void_v< IReturn_ >, void > {
        parent_response_ref.ref = std::ref(parent_resp_val);
    }

    ResponseValueRef<OReturn> own_response_ref;
    ResponseValueRef<IReturn> parent_response_ref;
    NullableRef<HierarFlowCallee<I>> next_node_ref;
    NullableRef<HierarFlowCallee<O>> first_succ_ref;
};
