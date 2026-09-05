#include "engine/memtable.hpp"

#include <random>
#include <algorithm>

namespace TSDB {

MemTable::MemTable() 
    : current_level_(1), size_bytes_(0), element_count_(0), dist_(0.0f, 1.0f) {
    
    // Seed RNG
    std::random_device rd;
    rng_.seed(rd());

    // Create sentinel dummy head node with max level height
    head_ = new SkipNode({}, {}, MAX_LEVEL);
    
    // Base tracking overhead
    size_bytes_ = sizeof(MemTable) + sizeof(SkipNode) + (MAX_LEVEL * sizeof(SkipNode*));    
}

MemTable::~MemTable() {
    clear();
    delete head_;
}

int MemTable::random_level() const {
    int lvl = 1;
    while (dist_(rng_) < P && lvl < MAX_LEVEL) {
        lvl++;
    }
    return lvl;
}

void MemTable::clear() {
    SkipNode* curr = head_->forward[0];
    while (curr != nullptr) {
        SkipNode* next = curr->forward[0];
        delete curr;
        curr = next;
    }

    // Reset forward pointers on Sentinel Head
    std::fill(head_->forward.begin(), head_->forward.end(), nullptr);

    current_level_ = 1;
    element_count_ = 0;
    size_bytes_ = sizeof(MemTable) + sizeof(SkipNode) + (MAX_LEVEL * sizeof(SkipNode*));
}

void MemTable::put(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value) {
    std::vector<SkipNode*> update(MAX_LEVEL, nullptr);
    SkipNode* curr = head_;

    // 1. Traverse down express lanes to locate predecessors per level
    for (int i = current_level_ - 1; i >= 0; --i) {
        while (curr->forward[i] != nullptr && curr->forward[i]->key < key) {
            curr = curr->forward[i];
        }
        update[i] = curr;
    }

    // Move to Level 0 target position
    curr = curr->forward[0];

    // 2. Check if key already exists (In-Place Overwrite)
    if (curr != nullptr && curr->key == key) {
        int64_t val_size_diff = static_cast<int64_t>(value.size()) - static_cast<int64_t>(curr->value.size());
        curr->value = value;
        size_bytes_ += val_size_diff;
        return;
    }

    // 3. Insert New Key
    int new_level = random_level();

    // If new level exceeds current height, adjust predecessors to point to head_
    if (new_level > current_level_) {
        for (int i = current_level_; i < new_level; ++i) {
            update[i] = head_;
        }
        current_level_ = new_level;
    }

    // Allocate node
    SkipNode* new_node = new SkipNode(key, value, new_level);

    // Splice forward pointers
    for (int i = 0; i < new_level; ++i) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    // Update memory tracking metrics
    element_count_++;
    size_bytes_ += sizeof(SkipNode) 
                 + key.size() 
                 + value.size() 
                 + (new_level * sizeof(SkipNode*));
}

bool MemTable::get(const std::vector<uint8_t>& key, std::vector<uint8_t>& value_out) const {
    SkipNode* curr = head_;

    // Traverse express lanes
    for (int i = current_level_ - 1; i >= 0; --i) {
        while (curr->forward[i] != nullptr && curr->forward[i]->key < key) {
            curr = curr->forward[i];
        }
    }

    curr = curr->forward[0];

    // Check exact match
    if (curr != nullptr && curr->key == key) {
        value_out = curr->value;
        return true;
    }

    return false;
}

std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> MemTable::scan(
    const std::vector<uint8_t>& start_key, 
    const std::vector<uint8_t>& end_key) const {
    
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> results;
    SkipNode* curr = head_;

    // 1. Traverse down to lower_bound start key
    for (int i = current_level_ - 1; i >= 0; --i) {
        while (curr->forward[i] != nullptr && curr->forward[i]->key < start_key) {
            curr = curr->forward[i];
        }
    }

    curr = curr->forward[0];

    // 2. Linear scan along Level 0 until end_key boundary is reached
    while (curr != nullptr && curr->key <= end_key) {
        results.emplace_back(curr->key, curr->value);
        curr = curr->forward[0];
    }

    return results;
}

} // namespace TSDB