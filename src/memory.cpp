#include "memory.h"
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <list>
#include <algorithm>

std::list<Memory::GuardHeader*> Memory::m_guarded_addrs;

void *Memory::alloc(size_t size) {
    return malloc(size);
}    

/*
    * Esta funcion esta destinada a llamarse en modo depuracion.
    *
    * Devuelve una trozo de memoria protegida en los extremos con un par de bytes marcados como <solo lectura>.
    * Su funcion es detectar intentos de escritura mas alla de los limites proporcionando un mensaje de error
    * mas esclarecedor que el tipico mensaje: "Segmentation fault".
    * 
    * El area protegida contra escritura se extiende _SC_PAGE_SIZE bytes en ambas direcciones (_SC_PAGE_SIZE * 2
    * bytes protegidos en total). La proteccion se aplica *por pagina*. El area de memoria sin proteccion tiene
    * un tamaño multiplo del tamaño de la pagina, por lo que el area total NO PROTEGIDA devuelta al usuario puede
    * puede ser mayor al tamaño solicitado. Esto impide la deteccion de escritura de los bytes de los bytes INMEDIATOS al final
    * del bloque de tamaño solicitado. Por lo que el desbordamiento en esta area no se detectara hasta que el usuario lea o escriba
    * en la siguiente pagina (protegida) o hasta que libere el bloque de memoria llamando a free() y comprobando
    * si la huella ha sido alterada.
*/
void *Memory::alloc_with_guard(const size_t size) {

    /* Redondeo hacia arriba al tamaño de pagina */
    const long ps = page_size();
    const size_t msize = round_to_page_size(size) + (ps * 2);

    char *addr = reinterpret_cast<char *>(
        mmap(nullptr, msize, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0));

    if (addr == MAP_FAILED) {
        return nullptr;
    }

    /* Utilizo los primeros bytes de la primer pagina para almacenar el tamaño total de ambos bloques */
    GuardHeader *header = reinterpret_cast<GuardHeader *>(addr);
    header->mmap_size = msize;
    header->blck_size = size;
    header->unpr_area = addr + ps + size;
    header->unpr_size = msize - ps*2 - size;

    /* Estampo una firma entre el final del tamaño del bloque solicitado por el usuario y la siguiente pagina (protegida)
     * para detectar cualquier escritura efectuada una vez se libera el bloque */
    stamp_signature(header->unpr_area, header->unpr_size);



    

    /* Protego contra lectura y escritra la primer y ultima pagina del bloque de memoria */
    //mprotect(addr, ps, PROT_NONE);
    //mprotect(addr + (msize - ps), ps, PROT_NONE);

    m_guarded_addrs.push_back(header);

    printf("Memory::alloc_with_guard -> %p\n", addr + ps);

    return addr + ps;
}

void Memory::free(void *ptr) {

    auto it = std::find(m_guarded_addrs.begin(), m_guarded_addrs.end(),
        get_header_from_user_pointer(ptr));

    if (it != m_guarded_addrs.end()) {
        free_guarded_addr(*it);
    }
    else {
        free(ptr);
    }
}

void Memory::stamp_signature(char *addr, size_t size) {
    /* TODO: Considerar usar una estampa diferente */
    memset(addr, 0xe1, size);
}

bool Memory::check_signature(char *addr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        /* El cast es necesario porque si el bit mas significativo esta activo el char se promocionara a int como
         * numero negativo */
        if ((unsigned char)addr[i] != 0xe1) {
            return false;
        }
    }
    return true;
}

long Memory::page_size() {
    return sysconf(_SC_PAGE_SIZE);
}

long Memory::round_to_page_size(size_t n) {
    const long ps = page_size();
    return (n % ps == 0) ?
        n :
        n + (ps - n % ps);
}

void Memory::free_guarded_addr(GuardHeader *header) {


    printf("Memory::free_guarded_addr( %p )\n", header);

    /* Primero quito las protecciones para no obtener un SIGSEGV al leer el tamaño del bloque de memoria */
    //mprotect(ptr, page_size(), PROT_READ);
    
    if (! check_signature(header->unpr_area, header->unpr_size)) {
        puts("Overflow...");
    }

    munmap(header, header->mmap_size);
}

Memory::GuardHeader *Memory::get_header_from_user_pointer(void *ptr) {
    return reinterpret_cast<GuardHeader *>(
        reinterpret_cast<char *>(ptr) - page_size());
}