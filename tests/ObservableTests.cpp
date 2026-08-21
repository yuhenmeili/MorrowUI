#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/Observable.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testPriorityAndStableOrder() {
    Observable<> observable;
    std::vector<int> calls;

    auto normalFirst = observable.connect([&]() { calls.push_back(1); });
    auto high = observable.connect([&]() { calls.push_back(2); }, 100);
    auto normalSecond = observable.connect([&]() { calls.push_back(3); });
    auto low = observable.connect([&]() { calls.push_back(4); }, -100);

    observable.notify();
    expect(calls == std::vector<int>({2, 1, 3, 4}),
           "priority should be descending and equal priorities should preserve connection order");
}

void testDisconnectDuringNotify() {
    Observable<> observable;
    std::vector<int> calls;
    Observable<>::Connection second;

    auto first = observable.connect([&]() {
        calls.push_back(1);
        second.disconnect();
    });
    second = observable.connect([&]() { calls.push_back(2); });

    observable.notify();
    expect(calls == std::vector<int>({1}),
           "disconnecting a pending observer should prevent it from running in the current notify");
    expect(observable.size() == 1, "disconnected observers should be compacted after notify");
}

void testDisconnectSelfDuringNotify() {
    Observable<> observable;
    int calls = 0;
    Observable<>::Connection connection;

    connection = observable.connect([&]() {
        ++calls;
        connection.disconnect();
    });

    observable.notify();
    observable.notify();
    expect(calls == 1, "an observer should be able to disconnect itself during notify");
}

void testConnectDuringNotifyStartsNextNotify() {
    Observable<> observable;
    std::vector<int> calls;
    Observable<>::Connection added;

    auto first = observable.connect([&]() {
        calls.push_back(1);
        if (!added.connected()) {
            added = observable.connect([&]() { calls.push_back(2); });
        }
    });

    observable.notify();
    expect(calls == std::vector<int>({1}),
           "observers connected during notify should not run in the current notify");

    observable.notify();
    expect(calls == std::vector<int>({1, 1, 2}),
           "observers connected during notify should run on the next notify");
}

void testConnectionLifetime() {
    Observable<> observable;
    int calls = 0;

    {
        auto connection = observable.connect([&]() { ++calls; });
        expect(connection.connected(), "new connection should report connected");
        observable.notify();
    }

    observable.notify();
    expect(calls == 1, "destroying a connection should automatically disconnect it");
    expect(observable.size() == 0, "expired RAII connections should not count as observers");
}

void testNestedNotify() {
    Observable<int> observable;
    std::vector<int> calls;

    auto connection = observable.connect([&](int value) {
        calls.push_back(value);
        if (value == 1) {
            observable.notify(2);
        }
    });

    observable.notify(1);
    expect(calls == std::vector<int>({1, 2}), "nested notify should remain valid");
}

void testObservableDestroyedBeforeConnection() {
    Observable<>::Connection connection;
    {
        auto observable = std::make_unique<Observable<>>();
        connection = observable->connect([]() {});
        expect(connection.connected(), "connection should be active while its observable exists");
    }

    expect(!connection.connected(), "connection should become inactive when its observable is destroyed");
    connection.disconnect();
}

void testClearInvalidatesConnections() {
    Observable<> observable;
    auto connection = observable.connect([]() {});

    observable.clear();
    expect(!connection.connected(), "clear should invalidate existing connections");
    expect(observable.size() == 0, "clear should remove all observers");
}

} // namespace

int main() {
    testPriorityAndStableOrder();
    testDisconnectDuringNotify();
    testDisconnectSelfDuringNotify();
    testConnectDuringNotifyStartsNextNotify();
    testConnectionLifetime();
    testNestedNotify();
    testObservableDestroyedBeforeConnection();
    testClearInvalidatesConnections();

    if (g_failures != 0) {
        std::cerr << g_failures << " Observable test(s) failed\n";
        return 1;
    }

    std::cout << "All Observable tests passed\n";
    return 0;
}
