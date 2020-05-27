#ifndef __ROBIN_HOOD_HASH_ROBIN_HOOD_HASH_2_H__
#define __ROBIN_HOOD_HASH_ROBIN_HOOD_HASH_2_H__

#include <stdint.h>
#include <string>
#include <cstring>
#include <functional>

namespace outils {

struct RobinHoodHash2Key final {
    RobinHoodHash2Key(const char* _base = nullptr, uint32_t _size = 0)
        : base(_base), size(_size) {}
    const char* base;
    uint32_t size;
};

template <uint32_t ValueSize,
          typename GetKeyFromValue,
          typename KeyHash,
          typename KeyEqual>
class RobinHoodHash2 final {
private:
    static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.9;

private:
    struct HashNode final {
        uint32_t psl;
        char value[ValueSize];
    } __attribute__((packed));

    struct HashTableInfo final {
        uint32_t key_num = 0;
        uint32_t lpsl = 0;
        uint32_t max_key_num = 0;
        uint32_t table_size = 0;
    } __attribute__((packed));

public:
    RobinHoodHash2() : m_is_mapping(false), m_table_info(nullptr), m_table(nullptr) {}

    ~RobinHoodHash2() {
        if (!m_is_mapping) {
            if (m_table_info) {
                free(m_table_info);
            }
        }
    }

    bool Init(uint32_t max_key_num, float max_load_factor = DEFAULT_MAX_LOAD_FACTOR) {
        if (m_table_info) {
            return false;
        }

        const uint32_t table_size = max_key_num / max_load_factor;

        m_table_info = (HashTableInfo*)malloc(sizeof(HashTableInfo) +
                                              table_size * sizeof(HashNode));
        if (!m_table_info) {
            return false;
        }

        m_table_info->key_num = 0;
        m_table_info->lpsl = 0;
        m_table_info->max_key_num = max_key_num;
        m_table_info->table_size = table_size;

        m_table = (HashNode*)((char*)m_table_info + sizeof(HashTableInfo));

        for (uint32_t i = 0; i < table_size; ++i) {
            MarkSlotEmpty(i);
        }

        return true;
    }

    bool Init(void* data, uint64_t size) {
        if (m_table_info) {
            return false;
        }

        if (size < sizeof(HashTableInfo)) {
            return false;
        }
        size -= sizeof(HashTableInfo);

        m_table_info = (HashTableInfo*)data;
        if (size < m_table_info->table_size * sizeof(HashNode)) {
            return false;
        }

        m_table = (HashNode*)((char*)m_table_info + sizeof(HashTableInfo));

        m_is_mapping = true;
        return true;
    }

    const char* Data() const {
        return m_table_info;
    }

    uint64_t Size() const {
        return sizeof(HashTableInfo) + m_table_info->table_size * sizeof(HashNode);
    }

    std::pair<char*, bool> Insert(const char* value) {
        HashNode node;
        node.psl = 0;
        memcpy(node.value, value, ValueSize);
        return InternalInsert(std::move(node));
    }

    char* Lookup(const RobinHoodHash2Key& key) {
        auto slot = InternalLookup(key);
        if (slot == UINT32_MAX) {
            return nullptr;
        }

        return m_table[slot].value;
    }

    const char* Lookup(const RobinHoodHash2Key& key) const {
        return const_cast<RobinHoodHash2*>(this)->Lookup(key);
    }

    void Remove(const RobinHoodHash2Key& key) {
        auto slot = InternalLookup(key);
        if (slot == UINT32_MAX) {
            return;
        }

        uint32_t next_slot = (slot + 1) % m_table_info->table_size;
        while (!SlotIsEmpty(next_slot) && m_table[next_slot].psl > 0) {
            memcpy(m_table[slot].value, m_table[next_slot].value, ValueSize);
            m_table[slot].psl = m_table[next_slot].psl - 1;

            slot = next_slot;
            next_slot = (next_slot + 1) % m_table_info->table_size;
        }

        MarkSlotEmpty(slot);
    }

    void ForEach(const std::function<bool (uint32_t slot, uint32_t psl, char* v)>& f) {
        if (m_table) {
            for (uint32_t i = 0; i < m_table_info->table_size; ++i) {
                if (!SlotIsEmpty(i)) {
                    if (!f(i, m_table[i].psl, m_table[i].value)) {
                        return;
                    }
                }
            }
        }
    }

    void ForEach(const std::function<bool (uint32_t slot, uint32_t psl, const char* v)>& f) const {
        const_cast<RobinHoodHash2*>(this)->ForEach([&f] (uint32_t slot, uint32_t psl, char* v) -> bool {
            return f(slot, psl, v);
        });
    }

private:
    bool SlotIsEmpty(uint32_t slot) const {
        return (m_table[slot].psl == UINT32_MAX);
    }

    void MarkSlotEmpty(uint32_t slot) {
        m_table[slot].psl = UINT32_MAX;
    }

    uint32_t InternalLookup(const RobinHoodHash2Key& key) const {
        uint32_t slot = m_hash(key) % m_table_info->table_size;

        for (uint32_t psl = 0; psl < m_table_info->lpsl; ++psl) {
            if (m_equal(key, m_get_key_from_value(m_table[slot].value))) {
                return slot;
            }

            slot = (slot + 1) % m_table_info->table_size;

            if (SlotIsEmpty(slot) || psl > m_table[slot].psl) {
                return UINT32_MAX;
            }
        }

        return UINT32_MAX;
    }

    std::pair<char*, bool> InternalInsert(HashNode&& tmp_node) {
        uint32_t slot = m_hash(m_get_key_from_value(tmp_node.value)) %
            m_table_info->table_size;

        while (true) {
            if (SlotIsEmpty(slot)) {
                if (m_table_info->key_num >= m_table_info->max_key_num) {
                    return std::pair<char*, bool>(nullptr, false);
                }

                m_table[slot].psl = tmp_node.psl;
                memcpy(m_table[slot].value, tmp_node.value, ValueSize);

                ++m_table_info->key_num;
                if (tmp_node.psl > m_table_info->lpsl) {
                    m_table_info->lpsl = tmp_node.psl;
                }
                return std::make_pair(m_table[slot].value, true);
            }

            if (m_equal(m_get_key_from_value(m_table[slot].value),
                        m_get_key_from_value(tmp_node.value))) {
                return std::make_pair(m_table[slot].value, false);
            }

            if (tmp_node.psl > m_table[slot].psl) {
                if (tmp_node.psl > m_table_info->lpsl) {
                    m_table_info->lpsl = tmp_node.psl;
                }
                std::swap(tmp_node, m_table[slot]);
            }

            ++tmp_node.psl;
            slot = (slot + 1) % m_table_info->table_size;
        }

        return std::pair<char*, bool>(nullptr, false);
    }

private:
    bool m_is_mapping;
    HashTableInfo* m_table_info;
    HashNode* m_table;
    KeyHash m_hash;
    KeyEqual m_equal;
    GetKeyFromValue m_get_key_from_value;

public:
    RobinHoodHash2(RobinHoodHash2&&) = default;
    RobinHoodHash2& operator=(RobinHoodHash2&&) = default;

private:
    RobinHoodHash2(const RobinHoodHash2&) = delete;
    RobinHoodHash2& operator=(const RobinHoodHash2&) = delete;
};

}

#endif
