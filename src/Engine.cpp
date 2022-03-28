//
// Created by Hirano Masahiro <masahiro.dll@gmail.com>
//

#include "Engine.h"
#include "AppMsg.h"
#include "Config.h"
#include "Logger.h"
#include "ZoomLensController.h"

namespace Bench {
    template <typename TimeT = std::chrono::milliseconds, typename F>
    inline TimeT take_time(F &&f) {
        const auto begin = std::chrono::high_resolution_clock::now();
        f();
        const auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<TimeT>(end - begin);
    }

    template<typename TimeT, typename F> struct BenchDelegate {
        static long delegatedBenchFunc(F&& f){
            const auto t = take_time<TimeT>(std::forward<F>(f));
            std::chrono::duration<long, std::milli> t_ = t;
            printf("%ld [ms]\n", t_.count());
            SPDLOG_INFO("{} [ms]", t_.count());
            return t_.count();
        }
    };

    template <typename TimeT = std::chrono::milliseconds, typename F>
    inline long bench(F &&f) {
        return BenchDelegate<TimeT, F>::delegatedBenchFunc(std::forward<F>(f));
    }

}


bool EngineOffline::run() {

    worker = std::thread([this] {
        workerStatus.store(WORKER_STATUS::RUNNING);

		ZoomLensController zlc(appMsg);
		zlc.run();

        workerStatus.store(WORKER_STATUS::IDLE);
    });

    SPDLOG_DEBUG("Image processing for a single frame done ...");

//    reset();

    return true;
}

bool EngineOffline::reset() {
    if (worker.joinable()) {
        worker.join();
    }
    return true;
}