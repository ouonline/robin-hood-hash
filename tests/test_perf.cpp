#include "robin-hood-hash/robin_hood_hash_1.h"
using namespace outils;

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
using namespace std;

#include <sys/time.h>
#include <cstring>

#define N 1999999
#define MAX_LOAD_FACTOR 0.9

static inline uint64_t DiffTimeMs(struct timeval end, const struct timeval& begin) {
    if (end.tv_usec < begin.tv_usec) {
        --end.tv_sec;
        end.tv_usec += 1000000;
    }

    return (end.tv_sec - begin.tv_sec) * 1000 +
        (end.tv_usec - begin.tv_usec) / 1000;
}

static const char* g_strs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

static inline void GenerateRandomData(vector<string>* vec) {
    int len = strlen(g_strs);

    srand(time(nullptr));
    for (int i = 0; i < N; ++i) {
        string s;
        int slen = rand() % 32 + 1;
        s.resize(slen);
        for (int j = 0; j < slen; ++j) {
            s[j] = g_strs[rand() % len];
        }
        vec->push_back(std::move(s));
    }
}

static void TestRobinHood(const vector<string>& data) {
    RobinHoodHashSet<string> st;
    st.Init(N, MAX_LOAD_FACTOR);

    struct timeval begin, end;

    gettimeofday(&begin, nullptr);
    for (auto s : data) {
        st.Insert(s);
    }
    gettimeofday(&end, nullptr);
    cout << "RobinHoodHash insert " << N << " strings costs " << DiffTimeMs(end, begin) << "ms." << endl;

    gettimeofday(&begin, nullptr);
    for (auto s : data) {
        st.Lookup(s);
    }
    gettimeofday(&end, nullptr);
    cout << "RobinHoodHash lookup " << N << " strings costs " << DiffTimeMs(end, begin) << "ms." << endl;
}

static void TestUnorderedSet(const vector<string>& data) {
    unordered_set<string> st;
    st.reserve(N);
    st.max_load_factor(MAX_LOAD_FACTOR);

    struct timeval begin, end;

    gettimeofday(&begin, nullptr);
    for (auto s : data) {
        st.insert(s);
    }
    gettimeofday(&end, nullptr);
    cout << "unordered_set insert " << N << " strings costs " << DiffTimeMs(end, begin) << "ms." << endl;

    gettimeofday(&begin, nullptr);
    for (auto s : data) {
        st.find(s);
    }
    gettimeofday(&end, nullptr);
    cout << "unordered_set find " << N << " strings costs " << DiffTimeMs(end, begin) << "ms." << endl;
}

int main(void) {
    vector<string> vec;
    GenerateRandomData(&vec);

    TestRobinHood(vec);
    TestUnorderedSet(vec);

    return 0;
}
