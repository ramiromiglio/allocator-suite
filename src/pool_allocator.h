#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cxxabi.h>
#include <assert.h>
#include <stdint.h>

#include <new>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <list>

//#include "memory.h"
#include "macros.h"

void *
mem_alloc(size_t size) { return malloc(size); }
void
mem_free(void *ptr) { free(ptr); }

template<typename T>
class PoolAllocator {

    struct Node {
        Node *prev;
        Node *next;
        ptrdiff_t index(Node *table) {
            return this - table;
        }
    };

public:
    typedef T  value_type;
    typedef T *pointer;

    PoolAllocator(const size_t size = 4096) : m_size(size) {
        ERR_FAIL_IF(size == 0, "El tamaño no puede ser cero");
        m_pool = reinterpret_cast<T *>    (mem_alloc(size * sizeof(T)));
        m_tab  = reinterpret_cast<Node *> (mem_alloc(size * sizeof(Node)));
        m_heap_allocated = true;
        construct();
    }

    ~PoolAllocator() {
        free_all();
        if (m_heap_allocated) {
            mem_free(m_pool);
            mem_free(m_tab);
            m_pool = nullptr;
            m_tab = nullptr;
        }
    }

    template<typename... Args>
    T *alloc(Args... args) {
        void *chunk = alloc_chunk();
        if (chunk) {
            return new (chunk) T(args...);
        }
        return nullptr;
    }

    T *alloc() {
        void *chunk = alloc_chunk();
        if (chunk) {
            return new (chunk) T;
        }
        return nullptr;
    }

    void free(T *chunk) {
        if (UNLIKELY(chunk == nullptr)) {
            return;
        }

        Node *node = m_tab + (chunk - m_pool);

#ifdef DEBUG_POOL_ALLOCATOR
        {
            /* Compruebo si el nodo ya fue liberado previamente */
            Node *next = m_head_free;
            for (; next != nullptr; next = next->next) {
                if (next == node) {
                    DEBUG_ERR("Free was called twice");
                    return;
                }
            }
        }
#endif

        chunk->~T();

        /* Elimino el nodo de la lista de nodos usados. Este se puede encontrar al comienzo, en el medio o al
         * final de la lista, por lo que manejo todos los casos posibles */
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
        if (m_head_used == node) {
            m_head_used = node->next;
        }

        /* Ahora inserto el nodo en la lista de nodos libres nuevamente */
        node->prev = nullptr;
        node->next = m_head_free;
        if (node->next) {
            node->next->prev = node;
        }
        m_head_free = node;
        m_chunks_used--;
        DEBUG_LOG("Free [%p]", chunk);
    }

    void free_all() {
        Node *node = m_head_used;
        if (! node) {
            return;
        }
        int count = 0;
        /* Destruyo los objetos en el orden inverso al que fueron creados */
        while (node != nullptr) {
            T *chunk = m_pool + node->index(m_tab);
            Node *next = node->next;
            free(chunk);
            node = next;
            count++;
        }
        DEBUG_LOG("Free all (%d in total)", count);
        ERR_VERIFY(m_chunks_used == 0);
    }

    size_t size()     const { return m_size; }
    size_t capacity() const { return m_size - m_chunks_used; }

PRIVATE_BUT_PUBLIC_FOR_TESTING:
    const size_t m_size;
    T      *m_pool = nullptr;
    Node   *m_tab  = nullptr;
    Node   *m_head_free = nullptr;
    Node   *m_head_used = nullptr;
    bool    m_heap_allocated = false;
    int     m_chunks_used = 0;

    void construct() {
        Node *node = m_tab;

        node->prev = nullptr;
        node->next = (m_size > 1) ? node + 1 : nullptr;

        while (++node != m_tab + m_size) {
            node->prev = node - 1;
            node->next = node + 1;
        }
        (--node)->next = nullptr;

        m_head_free = m_tab;
        m_head_used = nullptr;

#ifdef DEBUG_POOL_ALLOCATOR
        {
            int status;
            char *realname = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
            DEBUG_LOG("New PoolAllocator<%s> of size %d", realname, m_size);
            ::free(realname);
        }
#endif
    }

    FORCE_INLINE
    void *alloc_chunk() {
        if (UNLIKELY(m_head_free == nullptr)) {
            DEBUG_ERR("Insufficient storage");
            return nullptr;
        }

        /* Remuevo el nodo de la lista de nodos libres */
        Node *node = m_head_free;
        if (node->next) {
            node->next->prev = nullptr;
        }
        m_head_free = node->next;

        /* Asigno el nodo a la lista de nodos en uso */
        node->next = m_head_used;
        if (node->next) {
            node->next->prev = node;
        }
        m_head_used = node;

        ERR_VERIFY(node->prev == nullptr);

        m_chunks_used++;
        void *chunk = m_pool + node->index(m_tab);
        DEBUG_LOG("Alloc [%p]", chunk);
        return chunk;
    }
};