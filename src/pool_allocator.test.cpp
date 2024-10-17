#if 0

//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#define RANGE(from, to) for (int i = (from); i != (to); i += ((from) < (to) ? 1 : -1))

//#define DEBUG_POOL_ALLOCATOR
#define UNIT_TEST_ENABLED
#include "pool_allocator.h"

#include <stddef.h>
#include <vector>
#include <string>

ptrdiff_t bdiff(void *a, void *b) { return (char *)a - (char *)b; }

template<typename T>
void pool_allocator_test(const size_t size) {
    PoolAllocator<T> pool(size);
    std::vector<T*> p(size, nullptr);

    CHECK( pool.capacity() == size );

    RANGE( 0, size ) {
        p[i] = pool.alloc();
        CHECK( p[i] != nullptr );
        CHECK( pool.capacity() == size - i - 1 );
    }

    CHECK( pool.capacity() == 0 );
    CHECK( pool.alloc() == nullptr );
    
    if (size > 1) {
        CHECK( p[0] < p[1] );
        CHECK( p[0]+size-1 == p[size-1] );
        CHECK( bdiff( p[1]     , p[0]      ) == sizeof(T));
        CHECK( bdiff( p[size-1], p[size-2] ) == sizeof(T));

        CHECK(( (char *) p[2] - (char *) p[1] ) == sizeof(T));
        CHECK( p[1] > p[0] );
        CHECK( p[2] > p[1] );
    }

    pool.free( p[0] );
    pool.free( nullptr );
    CHECK( pool.capacity() == 1 );

    p[0] = pool.alloc();
    CHECK( pool.alloc() == nullptr );

    pool.free_all();
    CHECK( pool.capacity() == size );
}

struct raii_helper {
    static std::vector<int> destruction_order;
    raii_helper() {}
    ~raii_helper() {
        destruction_order.push_back(m_index);
    }
    void set_index(int index) {
        m_index = index;
    }
private:
    int m_index;
};
std::vector<int> raii_helper::destruction_order;

void pool_allocator_inverse_destruction_order() {

    const int size = 64;

    /* Encierro al pool entre llaves para que se destruya automaticamente al salir del alcance */    
    {
        PoolAllocator<raii_helper> pool(size);
        std::vector<raii_helper*> p(pool.size(), nullptr);

        RANGE(0, pool.size()) {
            p[i] = pool.alloc();
            p[i]->set_index(i);
        }
    }

    RANGE(0, size) {
        CHECK( raii_helper::destruction_order[i] == size - i - 1 );
    }
}

TEST_SUITE("Pool allocator") {
    TEST_CASE_TEMPLATE("Alloc and free capabilities", T, char, long long)            { pool_allocator_test<T>(1);     };
    TEST_CASE_TEMPLATE("Alloc and free capabilities", T, bool, double, void*)        { pool_allocator_test<T>(17);    };
    TEST_CASE_TEMPLATE("Alloc and free capabilities", T, int, long double, short***) { pool_allocator_test<T>(4790);  };
    TEST_CASE_TEMPLATE("Alloc and free capabilities", T, long, std::string)          { pool_allocator_test<T>(18647); };
    TEST_CASE("Order of destruction inverse to construction") { pool_allocator_inverse_destruction_order(); };
}

#endif