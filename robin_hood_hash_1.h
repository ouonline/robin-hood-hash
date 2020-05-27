#ifndef __ROBIN_HOOD_HASH_ROBIN_HOOD_HASH_1_H__
#define __ROBIN_HOOD_HASH_ROBIN_HOOD_HASH_1_H__

#include <stdint.h>
#include <string>
#include <functional>

namespace outils {

template <typename Key,
          typename Value,
          typename GetKeyFromValue,
          typename KeyHash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class RobinHoodHash1 final {
private:
    static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.9;

private:
    struct HashNode final {
        HashNode(const Value& v, uint32_t s) : psl(s), value(v) {}
        HashNode(Value&& v, uint32_t s) : psl(s), value(std::move(v)) {}
        HashNode(HashNode&& node) : psl(node.psl), value(std::move(node.value)) {}
        void operator=(HashNode&& node) {
            if (&node != this) {
                psl = node.psl;
                value = std::move(node.value);
            }
        }

        uint32_t psl;
        Value value;
    };

    struct HashTableInfo final {
        uint32_t key_num = 0;
        uint32_t lpsl = 0;
        uint32_t max_key_num = 0;
        uint32_t table_size = 0;
    };

public:
    RobinHoodHash1() : m_table(nullptr) {}

    ~RobinHoodHash1() {
        if (!m_table || m_table_info.key_num == 0) {
            return;
        }

        for (uint32_t i = 0; i < m_table_info.table_size; ++i) {
            if (!SlotIsEmpty(i)) {
                m_table[i].~HashNode();

                --m_table_info.key_num;
                if (m_table_info.key_num == 0) {
                    break;
                }
            }
        }

        free(m_table);
    }

    bool Init(uint32_t max_key_num, float max_load_factor = DEFAULT_MAX_LOAD_FACTOR) {
        uint32_t table_size = max_key_num / max_load_factor;

        m_table = (HashNode*)malloc(table_size * sizeof(HashNode));
        if (!m_table) {
            return false;
        }

        m_table_info.key_num = 0;
        m_table_info.lpsl = 0;
        m_table_info.max_key_num = max_key_num;
        m_table_info.table_size = table_size;

        for (uint32_t i = 0; i < table_size; ++i) {
            MarkSlotEmpty(i);
        }

        return true;
    }

    std::pair<Value*, bool> Insert(const Value& value) {
        return InternalInsert(HashNode(value, 0));
    }

    std::pair<Value*, bool> Insert(Value&& value) {
        return InternalInsert(HashNode(std::move(value), 0));
    }

    Value* Lookup(const Key& key) {
        auto slot = InternalLookup(key);
        if (slot == UINT32_MAX) {
            return nullptr;
        }

        return &m_table[slot].value;
    }

    const Value* Lookup(const Key& key) const {
        return const_cast<RobinHoodHash1*>(this)->Lookup(key);
    }

    void Remove(const Key& key) {
        auto slot = InternalLookup(key);
        if (slot == UINT32_MAX) {
            return;
        }

        uint32_t next_slot = (slot + 1) % m_table_info.table_size;
        while (!SlotIsEmpty(next_slot) && m_table[next_slot].psl > 0) {
            m_table[slot] = std::move(m_table[next_slot]);
            --m_table[slot].psl;

            slot = next_slot;
            next_slot = (next_slot + 1) % m_table_info.table_size;
        }

        m_table[slot].~HashNode();
        MarkSlotEmpty(slot);
    }

    void ForEach(const std::function<bool (uint32_t slot, uint32_t psl, Value* v)>& f) {
        if (m_table) {
            for (uint32_t i = 0; i < m_table_info.table_size; ++i) {
                if (!SlotIsEmpty(i)) {
                    if (!f(i, m_table[i].psl, &m_table[i].value)) {
                        return;
                    }
                }
            }
        }
    }

    void ForEach(const std::function<bool (uint32_t slot, uint32_t psl, const Value& v)>& f) const {
        const_cast<RobinHoodHash1*>(this)->ForEach([&f] (uint32_t slot, uint32_t psl, Value* v) -> bool {
            return f(slot, psl, *v);
        });
    }

private:
    bool SlotIsEmpty(uint32_t slot) const {
        return (m_table[slot].psl == UINT32_MAX);
    }

    void MarkSlotEmpty(uint32_t slot) {
        m_table[slot].psl = UINT32_MAX;
    }

    uint32_t InternalLookup(const Key& key) const {
        uint32_t slot = m_hash(key) % m_table_info.table_size;

        for (uint32_t psl = 0; psl < m_table_info.lpsl; ++psl) {
            if (m_equal(key, m_get_key_from_value(m_table[slot].value))) {
                return slot;
            }

            slot = (slot + 1) % m_table_info.table_size;

            if (SlotIsEmpty(slot) || psl > m_table[slot].psl) {
                return UINT32_MAX;
            }
        }

        return UINT32_MAX;
    }

    std::pair<Value*, bool> InternalInsert(HashNode&& tmp_node) {
        uint32_t slot = m_hash(m_get_key_from_value(tmp_node.value)) % m_table_info.table_size;

        while (true) {
            if (SlotIsEmpty(slot)) {
                if (m_table_info.key_num >= m_table_info.max_key_num) {
                    return std::pair<Value*, bool>(nullptr, false);
                }

                new (m_table + slot) HashNode(std::move(tmp_node));
                ++m_table_info.key_num;
                if (tmp_node.psl > m_table_info.lpsl) {
                    m_table_info.lpsl = tmp_node.psl;
                }
                return std::make_pair(&m_table[slot].value, true);
            }

            if (m_equal(m_get_key_from_value(m_table[slot].value),
                        m_get_key_from_value(tmp_node.value))) {
                return std::make_pair(&m_table[slot].value, false);
            }

            if (tmp_node.psl > m_table[slot].psl) {
                if (tmp_node.psl > m_table_info.lpsl) {
                    m_table_info.lpsl = tmp_node.psl;
                }
                std::swap(tmp_node, m_table[slot]);
            }

            ++tmp_node.psl;
            slot = (slot + 1) % m_table_info.table_size;
        }

        return std::pair<Value*, bool>(nullptr, false);
    }

private:
    HashNode* m_table;
    HashTableInfo m_table_info;
    KeyHash m_hash;
    KeyEqual m_equal;
    GetKeyFromValue m_get_key_from_value;

public:
    RobinHoodHash1(RobinHoodHash1&&) = default;
    RobinHoodHash1& operator=(RobinHoodHash1&&) = default;

private:
    RobinHoodHash1(const RobinHoodHash1&) = delete;
    RobinHoodHash1& operator=(const RobinHoodHash1&) = delete;
};

/* -------------------------------------------------------------------------- */

namespace internal {

template <typename Key, typename Value>
struct ReturnFirstOfPairForMap final {
    const Key& operator () (const std::pair<Key, Value>& kv) const {
        return kv.first;
    }
};

template <typename Key>
struct ReturnSelfForSet final {
    const Key& operator () (const Key& key) const {
        return key;
    }
};

}

template <typename Key, typename Value,
          typename KeyHash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using RobinHoodHashMap = RobinHoodHash1<Key, std::pair<Key, Value>,
                                        internal::ReturnFirstOfPairForMap<Key, Value>,
                                        KeyHash, KeyEqual>;

template <typename Key,
          typename KeyHash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using RobinHoodHashSet = RobinHoodHash1<Key, Key,
                                        internal::ReturnSelfForSet<Key>,
                                        KeyHash, KeyEqual>;

}

#endif
