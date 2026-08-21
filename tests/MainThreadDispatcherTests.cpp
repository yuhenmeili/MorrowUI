#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "core/MainThreadDispatcher.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testPostAndDrain() {
    MainThreadDispatcher dispatcher;
    int wakeCount = 0;
    int value = 0;
    dispatcher.setWakeCallback([&]() { ++wakeCount; });

    expect(dispatcher.post([&]() { value += 1; }), "valid task should be accepted");
    expect(dispatcher.post([&]() { value += 2; }), "second valid task should be accepted");
    expect(wakeCount == 2, "each accepted post should invoke the wake callback");
    expect(dispatcher.pendingTaskCount() == 2, "posted tasks should remain pending until drain");
    expect(dispatcher.drain() == 2, "drain should report executed task count");
    expect(value == 3, "drain should execute tasks in FIFO order");
    expect(dispatcher.pendingTaskCount() == 0, "drain should consume pending tasks");
}

void testTasksPostedDuringDrainRunNextTime() {
    MainThreadDispatcher dispatcher;
    std::vector<int> calls;

    dispatcher.post([&]() {
        calls.push_back(1);
        dispatcher.post([&]() { calls.push_back(2); });
    });

    expect(dispatcher.drain() == 1, "current drain should only execute its initial snapshot");
    expect(calls == std::vector<int>({1}), "nested post should not execute in the current drain");
    expect(dispatcher.pendingTaskCount() == 1, "nested post should remain queued");
    expect(dispatcher.drain() == 1, "next drain should execute the nested post");
    expect(calls == std::vector<int>({1, 2}), "nested post should execute on the next drain");
}

void testMultipleProducers() {
    MainThreadDispatcher dispatcher;
    std::atomic<int> executed{0};
    std::vector<std::thread> producers;

    constexpr int producerCount = 4;
    constexpr int tasksPerProducer = 50;
    for (int producer = 0; producer < producerCount; ++producer) {
        producers.emplace_back([&]() {
            for (int task = 0; task < tasksPerProducer; ++task) {
                dispatcher.post([&]() { executed.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    for (auto& producer : producers) {
        producer.join();
    }

    expect(dispatcher.pendingTaskCount() == producerCount * tasksPerProducer,
           "all producer tasks should be queued");
    expect(dispatcher.drain() == producerCount * tasksPerProducer,
           "drain should execute every producer task once");
    expect(executed.load(std::memory_order_relaxed) == producerCount * tasksPerProducer,
           "multi-producer tasks should not be lost");
}

void testTaskFailureDoesNotStopDrain() {
    MainThreadDispatcher dispatcher;
    int calls = 0;

    dispatcher.post([]() { throw std::runtime_error("expected test failure"); });
    dispatcher.post([&]() { ++calls; });

    expect(dispatcher.drain() == 2, "failed tasks should still count as consumed");
    expect(calls == 1, "one failed task should not prevent later tasks from running");
}

void testShutdown() {
    MainThreadDispatcher dispatcher;
    int calls = 0;
    dispatcher.post([&]() { ++calls; });

    dispatcher.shutdown();
    expect(!dispatcher.acceptingTasks(), "shutdown should stop task acceptance");
    expect(dispatcher.pendingTaskCount() == 0, "shutdown should discard pending tasks");
    expect(!dispatcher.post([&]() { ++calls; }), "post after shutdown should be rejected");
    expect(dispatcher.drain() == 0, "shutdown dispatcher should have no tasks to drain");
    expect(calls == 0, "discarded tasks should not run");
}

} // namespace

int main() {
    testPostAndDrain();
    testTasksPostedDuringDrainRunNextTime();
    testMultipleProducers();
    testTaskFailureDoesNotStopDrain();
    testShutdown();

    if (g_failures != 0) {
        std::cerr << g_failures << " MainThreadDispatcher test(s) failed\n";
        return 1;
    }

    std::cout << "All MainThreadDispatcher tests passed\n";
    return 0;
}
