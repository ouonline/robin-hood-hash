#include "robin-hood-hash/robin_hood_hash_2.h"
#include <iostream>
using namespace std;
using namespace outils;

struct ReturnSelf final {
    RobinHoodHash2Key operator () (const char* v) const {
        return RobinHoodHash2Key(v, sizeof(int));
    }
};

struct KeyHash final {
    uint64_t operator () (const RobinHoodHash2Key& key) const {
        return *(int*)key.base;
    }
};

struct KeyEqual final {
    bool operator () (const RobinHoodHash2Key& k1,
                      const RobinHoodHash2Key& k2) const {
        return *(int*)k1.base == *(int*)k2.base;
    }
};

static inline void Print(const RobinHoodHash2<sizeof(int), ReturnSelf, KeyHash, KeyEqual>& t) {
    cout << "------------------------------------" << endl;
    t.ForEach([] (uint32_t slot, uint32_t psl, const char* v) -> bool {
        cout << "[" << slot << "] -> " << *(int*)v
             << ", psl = " << psl << endl;
        return true;
    });
    cout << "------------------------------------" << endl;
}

#define N 10

int main(void) {
    RobinHoodHash2<sizeof(int), ReturnSelf, KeyHash, KeyEqual> t;
    t.Init(N);

    int tmp_value;
    int values[] = {12, 24, 37, 36, 15, 27, 0};
    const unsigned value_size = sizeof(values) / sizeof(int);

    for (unsigned i = 0; i < value_size; ++i) {
        t.Insert((const char*)&values[i]);
        cout << "insert " << values[i] << endl;
        Print(t);
    }

    tmp_value = 24;
    t.Remove((const char*)&tmp_value);
    cout << "remove 24" << endl;
    Print(t);

    tmp_value = 12;
    t.Remove((const char*)&tmp_value);
    cout << "remove 12" << endl;
    Print(t);

    tmp_value = 36;
    t.Remove((const char*)&tmp_value);
    cout << "remove 36" << endl;
    Print(t);

    tmp_value = 48;
    t.Insert((const char*)&tmp_value);
    cout << "insert 48" << endl;
    Print(t);

    return 0;
}
