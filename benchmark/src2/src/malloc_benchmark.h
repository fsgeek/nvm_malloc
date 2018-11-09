#pragma once

#include <std>

class malloc_parameters {
    public:
        malloc_factory() {};
        ~malloc_factory() {};

        std::string get_backing_path(void) {
            return m_backing_path;
        }

        std::string get_allocator(void) {
            return m_allocator;
        }

        unsigned get_thread_count(void) {
            return m_threads;
        }

        unsigned get_minsize(void) {
            return m_minsize;
        }

        unsigned get_maxsize(void) {
            return m_maxsize;
        }

        unsigned get_verbose(void) {
            return m_verbose;
        }

        void set_backing_path(std::string backing_path) {
            m_backing_path = backing_path;
        }

        void set_alloctor(std::string allocator) {
            m_allocator = allocator;
        }

        void set_thread_count(unsigned thread_count) {
            m_threads = thread_count;
        }

        void set_minsize(size_t minsize) {
            m_minsize = minsize;
        }

        void set_maxsize(size_t maxsize) {
            m_maxsize = maxsize;
        }

        void set_verbose(bool verbose) {
            m_verbose = verbose;
        }

    private:
        std::string m_backing_path;
        std::string m_allocator;
        unsigned m_threads = 8;
        size_t m_minsize = 64;
        size_t m_maxsize = 64;
        bool m_verbose = false;

}

class malloc_allocator {
    public:
        malloc_allocator() = 0;
        ~malloc_allocator() {};

        void worker(int id) = 0;

};

class malloc_factory {
    public:
        malloc_factory() {};
        ~malloc_factory() {};

        typedef 

        static void RegisterAllocator(std::string allocator_name, malloc_allocator allocator);

        malloc_allocator FindAllocator(std::string allocator_name);

    private:
        map<std::string, malloc_allocator, 
        template < class Key,                                     // map::key_type
           class T,                                       // map::mapped_type
           class Compare = less<Key>,                     // map::key_compare
           class Alloc = allocator<pair<const Key,T> >    // map::allocator_type
           > class map;

};


class malloc_worker {
    public:
        malloc_worker();
        ~malloc_worker();
    
        void set_allocation_size_min(uint64_t min) { m_allocation_size_min = min; }
        void set_allocation_size_max(uint64_t max) { m_allocation_size_max = max; }

        virtual void worker(int id) = 0;

    private:
        std::vector<uint64_t> m_workerTimes;
        uint64_t m_allocation_size_min = 64;
        uint64_t m_allocation_size_max = 64;
};

