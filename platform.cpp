#include "platform.hpp"

#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif
#include <thread>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include "errors.hpp"

long get_num_avail_cpus() {
    return std::thread::hardware_concurrency();
}

#ifdef _WIN32
long get_page_size() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<long>(si.dwPageSize);
}
#else
long get_page_size() {
    return sysconf(_SC_PAGESIZE);
}
#endif

size_t calc_memsize() {
#ifdef _WIN32

    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);

    if (!GlobalMemoryStatusEx(&statex)) {
        std::cerr << "GlobalMemoryStatusEx failed\n";
        std::exit(EXIT_MEMORY);
    }

    return static_cast<size_t>(statex.ullTotalPhys);

#elif defined(__APPLE__)

    int64_t hw_memsize;
    size_t len = sizeof(hw_memsize);

    if (sysctlbyname("hw.memsize", &hw_memsize, &len, nullptr, 0) < 0) {
        perror("sysctl hw.memsize");
        std::exit(EXIT_MEMORY);
    }

    return static_cast<size_t>(hw_memsize);

#else

    long long pagesize = sysconf(_SC_PAGESIZE);
    long long pages = sysconf(_SC_PHYS_PAGES);

    if (pages < 0 || pagesize < 0) {
        perror("sysconf _SC_PAGESIZE or _SC_PHYS_PAGES");
        std::exit(EXIT_MEMORY);
    }

    return static_cast<size_t>(pages * pagesize);

#endif
}

#ifdef _WIN32
size_t get_max_open_files() {
    return 16384;
}
#else
size_t get_max_open_files() {
	struct rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
		perror("getrlimit");
		exit(EXIT_PTHREAD);
	}
	return rl.rlim_cur;
}
#endif
