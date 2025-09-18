// g++ --std=c++11 fds.cpp -o ./fds -Wno-deprecated-declarations

#include <sys/resource.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

// ...

bool checkFdLimit() noexcept {
#if !defined(_WIN32) && !defined(_WIN64)
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        printf("Failed to get RLIMIT_NOFILE: %s", strerror(errno));
        return false;
    }

    rlim_t currentLimit = rl.rlim_cur; // текущий "мягкий" лимит
    rlim_t maxLimit = rl.rlim_max;     // "жёсткий" лимит (требует root для повышения)

	uint32_t _maxCount = 4096;

	std::cout << " !!!! " << maxLimit << std::endl;

    if (_maxCount > currentLimit) {
            printf(
                "Requested max FDs (%u) exceeds current system limit (%lu). "
                "Consider increasing with 'ulimit -n %u' or as root: 'ulimit -n %u && your_app'.",
                _maxCount,
                static_cast<unsigned long>(currentLimit),
                _maxCount,
                _maxCount
            );
        // Можно не прерывать, но предупредить
        // return false; — если хочешь жёстко запретить запуск
    }

    return true;
#else
    // Windows — см. ниже
    return true;
#endif
}

int main(void){
	checkFdLimit();
	return 0;
}
