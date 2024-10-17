#include <stddef.h>
#include <list>

class Memory {
    struct GuardHeader {
        size_t mmap_size;
        size_t blck_size;
        char  *unpr_area;
        size_t unpr_size;
    };
public:
    static void *alloc(size_t size);
    static void *alloc_with_guard(const size_t size);
    static void free(void *ptr);

private:
    static std::list<GuardHeader*> m_guarded_addrs;

    static void stamp_signature(char *addr, size_t size);
    static bool check_signature(char *addr, size_t size);
    static long page_size();
    static long round_to_page_size(size_t n);
    static void free_guarded_addr(GuardHeader *header);
    static GuardHeader *get_header_from_user_pointer(void *ptr);
};