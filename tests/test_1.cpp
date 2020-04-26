#include <iostream>
using namespace std;

#include "robin-hood-hash/robin_hood_hash_1.h"
using namespace outils;

#define N 10

static inline void Print(const RobinHoodHashSet<int>& st) {
    cout << "------------------------------------" << endl;
    st.ForEach([] (uint32_t slot, uint32_t psl, const int& v) -> bool {
        cout << "[" << slot << "] -> " << v
             << ", psl = " << psl << endl;
        return true;
    });
    cout << "------------------------------------" << endl;
}

int main(void) {
    RobinHoodHashSet<int> st;
    st.Init(N);

    int values[] = {12, 24, 37, 36, 15, 27, 0};
    const unsigned value_size = sizeof(values) / sizeof(int);

    for (unsigned i = 0; i < value_size; ++i) {
        st.Insert(values[i]);
        cout << "insert " << values[i] << endl;
        Print(st);
    }

    st.Remove(24);
    cout << "remove 24" << endl;
    Print(st);

    st.Remove(12);
    cout << "remove 12" << endl;
    Print(st);

    st.Remove(36);
    cout << "remove 36" << endl;
    Print(st);

    st.Insert(48);
    cout << "insert 48" << endl;
    Print(st);

    return 0;
}
