#include "ExecInput/ExecInputSet.hpp"

ExecInputSet::ExecInputSet() {}

ExecInputSet::~ExecInputSet() {}

size_t ExecInputSet::size(void) {
    return elems.size();
}

NullableRef<ExecInput> ExecInputSet::get_ref(u64 id) {
    auto itr = elems.find(id);
    if (itr == elems.end()) return std::nullopt;
    return *itr->second;
}

std::shared_ptr<ExecInput> ExecInputSet::get_shared(u64 id) {
    auto itr = elems.find(id);
    if (itr == elems.end()) return nullptr;
    return itr->second;
}

void ExecInputSet::erase(u64 id) {
    auto itr = elems.find(id);
    if (itr == elems.end()) return;

    elems.erase(itr);
}

std::vector<u64> ExecInputSet::get_ids(void) {
    std::vector<u64> ids;
    for (auto& itr : elems) {
        ids.emplace_back(itr.first);
    }
    return ids;
}
