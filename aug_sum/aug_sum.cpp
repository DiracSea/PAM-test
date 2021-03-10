#include "pbbs-include/get_time.h"
#include "pbbs-include/parse_command_line.h"
timer t;

#include <cilk/cilk.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <random>
#include <cstdio>
#include <set>
#include <map>
#include "pam.h"
#include "pbbs-include/sequence_ops.h"
#include "pbbs-include/random_shuffle.h"

using namespace std;
//#define NO_AUG 1
#define LARGE 1

#ifdef LARGE
using key_type = size_t;
#else
using key_type = unsigned int;
#endif

struct entry
{
    using key_t = key_type;
    using val_t = key_type;
    using aug_t = key_type;
    static inline bool comp(key_t a, key_t b) { return a < b; }
    static aug_t get_empty() { return 0; }
    static aug_t from_entry(key_t k, val_t v) { return v; }
    static aug_t combine(aug_t a, aug_t b) { return std::max(a, b); }
};

struct entry2
{
    using key_t = key_type;
    using val_t = bool;
    static inline bool comp(key_t a, key_t b) { return a < b; }
};
struct entry3
{
    using key_t = key_type;
    using val_t = char;
    static inline bool comp(key_t a, key_t b) { return a < b; }
};

using par = pair<key_type, key_type>;

#ifdef NO_AUG
using tmap = pam_map<entry>;
#else
using tmap = aug_map<entry>;
#endif

struct mapped
{
    key_type k, v;
    mapped(key_type _k, key_type _v) : k(_k), v(_v){};
    mapped(){};

    bool operator<(const mapped &m)
        const { return k < m.k; }

    bool operator>(const mapped &m)
        const { return k > m.k; }

    bool operator==(const mapped &m)
        const { return k == m.k; }
};

size_t str_to_int(char *str)
{
    return strtol(str, NULL, 10);
}

std::mt19937_64 &get_rand_gen()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 generator(rd());
    return generator;
}

par *uniform_input(size_t n, size_t window, bool shuffle = false)
{
    par *v = new par[n];

    parallel_for(size_t i = 0; i < n; i++)
    {
        uniform_int_distribution<> r_keys(1, window);
        key_type k = r_keys(get_rand_gen());
        key_type c = i; //r_keys(get_rand_gen());
        v[i] = make_pair(k, c);
    }

    auto addfirst = [](par a, par b) -> par { return par(a.first + b.first, b.second); };
    auto vv = sequence<par>(v, n);
    pbbs::scan(vv, vv, addfirst, par(0, 0), pbbs::fl_scan_inclusive);
    if (shuffle)
        pbbs::random_shuffle(vv);
    return v;
}

par *uniform_input_unsorted(size_t n, size_t window)
{
    par *v = new par[n];

    parallel_for(size_t i = 0; i < n; i++)
    {
        uniform_int_distribution<> r_keys(1, window);

        key_type k = r_keys(get_rand_gen());
        key_type c = r_keys(get_rand_gen());

        v[i] = make_pair(k, c);
    }

    return v;
}

double test_union(size_t n, size_t m)
{

    par *v1 = uniform_input(n, 20);
    tmap m1(v1, v1 + n);

    par *v2 = uniform_input(m, (n / m) * 20);
    tmap m2(v2, v2 + m);
    double tm;

    //for (int i=0; i < 20; i++) {
    timer t;
    t.start();
    tmap m3 = tmap::map_union((tmap)m1, (tmap)m2);
    tm = t.stop();
    //cout << "time: " << tm << endl;
    //}

    assert(m1.size() == n && "map size is wrong.");
    assert(m2.size() == m && "map size is wrong.");
    assert(check_union(m1, m2, m3) && "union is wrong");

    delete[] v1;
    delete[] v2;

    tmap::finish();
    return tm;
}

double test_intersect(size_t n, size_t m)
{
    par *v1 = uniform_input(n, 2);
    tmap m1(v1, v1 + n);

    par *v2 = uniform_input(m, (n / m) * 2);
    tmap m2(v2, v2 + m);

    timer t;
    t.start();
    tmap m3 = tmap::map_intersect(m1, m2);
    double tm = t.stop();

    assert(m1.size() == n && "map size is wrong");
    assert(m2.size() == m && "map size is wrong");
    assert(check_intersect(m1, m2, m3) && "intersect is wrong");

    delete[] v1;
    delete[] v2;

    return tm;
}

double test_deletion(size_t n, size_t m)
{
    par *v = uniform_input(n, 20);
    tmap m1(v, v + n);

    par *u = uniform_input(m, (n / m) * 20, true);

    timer t;
    t.start();
    for (size_t i = 0; i < m; ++i)
        m1 = tmap::remove(move(m1), u[i].first);
    double tm = t.stop();

    delete[] v;
    delete[] u;
    return tm;
}

double test_deletion_destroy(size_t n)
{
    par *v = uniform_input(n, 20, true);
    tmap m1;
    for (size_t i = 0; i < n; ++i)
        m1.insert(v[i]);
    pbbs::random_shuffle(sequence<par>(v, n));

    timer t;
    t.start();
    for (size_t i = 0; i < n; ++i)
        m1 = tmap::remove(move(m1), v[i].first);
    double tm = t.stop();

    delete[] v;
    return tm;
}

double test_insertion_build(size_t n)
{
    par *v = uniform_input(n, 20, true);
    tmap m1;

    timer t;
    //freopen("int.txt", "w", stdout);
    t.start();
    for (size_t i = 0; i < n; ++i)
    {
        m1.insert(v[i]);
        //cout << v[i].first << " " << v[i].second << endl;
    }
    double tm = t.stop();

    delete[] v;
    return tm;
}

double test_insertion(size_t n, size_t m)
{
    par *v = uniform_input(n, 20);
    tmap m1(v, v + n);

    par *u = uniform_input(m, (n / m) * 20, true);

    timer t;
    t.start();
    for (size_t i = 0; i < m; ++i)
        m1.insert(u[i]);

    double tm = t.stop();
    delete[] v;
    delete[] u;
    return tm;
}

double test_build(size_t n)
{
    par *v = uniform_input_unsorted(n, 1000000000);

    timer t;
    t.start();
    tmap m1(v, v + n);
    double tm = t.stop();

    assert(m1.size() == n && "map size is wrong");
    assert(contains(m1, v) && "build is wrong");

    delete[] v;
    return tm;
}

double test_difference(size_t n, size_t m)
{
    par *v1 = uniform_input(n, 20);
    tmap m1(v1, v1 + n);

    par *v2 = uniform_input(m, (n / m) * 20);
    tmap m2(v2, v2 + m);

    timer t;
    t.start();
    tmap m3 = tmap::map_difference(m1, m2);
    double tm = t.stop();

    assert(m1.size() == n && "map size is wrong");
    assert(m2.size() == m && "map size is wrong");
    assert(check_difference(m1, m2, m3));

    delete[] v1;
    delete[] v2;

    return tm;
}

inline uint32_t hash64(uint32_t a)
{
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    if (a < 0)
        a = -a;
    return a;
}

void rand(size_t *A, size_t n)
{
    for (size_t i = 0; i < n; i++)
        A[i] = i;
    for (size_t i = n - 1; i; i--)
    {
        swap(A[i], A[hash64(i) % (i + 1)]);
    }
}

par *rand_input(size_t n, size_t *key)
{
    par *v = new par[n];
    parallel_for(0, n, [&](size_t i) {
        key_type k = key[i];
        key_type c = i; //r_keys(get_rand_gen());
        v[i] = make_pair(k, c);
    });
    return v;
}

int main(int argc, char *argv[])
{
    int c = atoi(argv[1]);
    int d = atoi(argv[2]);
    size_t n = 10000000, m = 100000000;
    size_t *A = new size_t[n];
    size_t *B = new size_t[m];
    size_t *C = new size_t[m];
    par *v1, *v2;

    switch (d)
    {
    case 0: // sorted
        cout << "sorted:" << endl;
        for (size_t i = 0; i < n; i++)
            A[i] = i + 1;
        v1 = rand_input(n, A);
        rand(A, n);
        v2 = rand_input(n, A);
        break;
    case 1: // reversed
        cout << "reversed:" << endl;
        for (size_t i = 0; i < n; i++)
            A[i] = n - i;
        v1 = rand_input(n, A);
        rand(A, n);
        v2 = rand_input(n, A);
        break;
    case 2: // rand
    case 10:
        cout << "rand:" << endl;
        for (size_t i = 0; i < n; i++)
            A[i] = i + 1;
        rand(A, n);
        v1 = rand_input(n, A);
        v2 = v1;
        break;
    case 3: // union
        cout << "union:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = 2 * i;
        for (size_t i = 0; i < m; i++)
            C[i] = 2 * i + 1;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    case 8: // union half
        cout << "union half:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + m / 2;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    case 4: // intersect 1/2
        cout << "intersect half:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + m / 2;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    case 5: // intersect 1
        cout << "intersect:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + 1;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    case 6: // difference 1/2
        cout << "difference half:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + m / 2;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    case 7: // difference 1
        cout << "difference:" << endl;
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + 1;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        break;
    default:
        for (size_t i = 0; i < m; i++)
            B[i] = i + 1;
        for (size_t i = 0; i < m; i++)
            C[i] = i + 1;
        v1 = rand_input(m, B);
        v2 = rand_input(m, C);
        return 0;
    }
    cout << "set:" << endl;

    tmap m1, m4;
    timer t, t1, t2;
    switch (d)
    {
    case 0:
    case 1:
    case 2:
    {
        t.start();
        for (size_t i = 0; i < n; i++)
            m1.insert(v1[i]);
        t.stop();
        cout << "insert:" << t.get_total() << endl;
        t1.start();
        for (size_t i = 0; i < n; i++)
            m1 = tmap::remove(move(m1), v2[i].first);
        t1.stop();
        cout << "delete:" << t1.get_total() << endl;
        break;
    }
    case 3:
    case 8:
    {
        tmap m2(v1, v1 + m);
        tmap m3(v2, v2 + m);
        t2.start();
        m4 = tmap::map_union((tmap)m2, (tmap)m3);
        t2.stop();
        cout << "set-set time:" << t2.get_total() << endl;
        break;
    }
    case 4:
    case 5:
    {
        tmap m2(v1, v1 + m);
        tmap m3(v2, v2 + m);
        t2.start();
        m4 = tmap::map_intersect((tmap)m2, (tmap)m3);
        t2.stop();
        cout << "set-set time:" << t2.get_total() << endl;
        break;
    }
    case 6:
    case 7:
    {
        tmap m2(v1, v1 + m);
        tmap m3(v2, v2 + m);
        t2.start();
        m4 = tmap::map_difference((tmap)m2, (tmap)m3);
        t2.stop();
        cout << "set-set time:" << t2.get_total() << endl;
        break;
    }
    case 10:
    {
        t2.start();
        tmap m5(v1, v1 + n);
        t2.stop();
        cout << "build time:" << t2.get_total() << endl;
        break;
    }
    default:
        return 0;
    }
}
