#include <iostream>
#include <string>

#include "ui/base/BaseButton.h"
#include "ui/elements/MRItemList.h"
#include "ui/elements/MRTree.h"
#include "ui/elements/MRVideoStreamPlayer.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void click(const std::shared_ptr<BaseButton>& button) {
    button->onMouseDown();
    button->onMouseUp();
}

void testItemListSelection() {
    auto list = MRItemList::create();
    list->addItem(L"First", 10);
    list->addItem(L"Second", 20);
    bool callbackCalled = false;
    auto selectionConnection =
        list->events().onItemSelected.connect(
            [&](MRItemList&, int id, const std::wstring&) {
                callbackCalled = id == 20;
            });

    click(std::dynamic_pointer_cast<MRButton>(list->m_children[1]));
    expect(list->getSelectedId() == 20, "item list should store the selected id");
    expect(callbackCalled, "item list should invoke its selection callback");
}

void testTreeExpansionAndSelection() {
    auto tree = MRTree::create();
    expect(tree->addNode(1, L"Root"), "tree should accept a root node");
    expect(tree->addNode(2, L"Child", 1), "tree should accept a child node");
    expect(!tree->addNode(2, L"Duplicate"), "tree should reject duplicate ids");
    expect(tree->setExpanded(1, false), "tree should collapse a parent node");
    expect(!tree->isExpanded(1), "tree should expose the collapsed state");
    expect(tree->selectNode(2), "tree should allow selecting a hidden child by id");
    expect(tree->getSelectedId() == 2, "tree should store the selected node id");
}

void testVideoPlayerControls() {
    auto player = MRVideoStreamPlayer::create(8, 4, 10.0f, 20);
    int providerCalls = 0;
    player->setFrameProvider([&](uint64_t frame, std::vector<unsigned char>& pixels) {
        ++providerCalls;
        std::fill(pixels.begin(), pixels.end(), static_cast<unsigned char>(frame));
        return true;
    });

    expect(providerCalls == 1, "setting the frame provider should present the initial frame");
    player->seekFrame(7);
    expect(player->getCurrentFrame() == 7, "video player should seek by frame");
    player->seekSeconds(1.2);
    expect(player->getCurrentFrame() == 12, "video player should seek by seconds");
    player->setLoop(true);
    expect(player->isLoop(), "video player should store the loop flag");
    player->setPlaybackSpeed(2.0f);
    expect(player->getPlaybackSpeed() == 2.0f, "video player should store playback speed");
    player->play();
    expect(player->getPlaybackState() == MRVideoStreamPlayer::PlaybackState::PLAYING,
           "video player should enter playing state");
    player->pause();
    expect(player->getPlaybackState() == MRVideoStreamPlayer::PlaybackState::PAUSED,
           "video player should enter paused state");
    player->stop();
    expect(player->getPlaybackState() == MRVideoStreamPlayer::PlaybackState::STOPPED &&
               player->getCurrentFrame() == 0,
           "stopping video should reset it to frame zero");
}

}  // namespace

int main() {
    testItemListSelection();
    testTreeExpansionAndSelection();
    testVideoPlayerControls();

    if (g_failures != 0) {
        std::cerr << g_failures << " item/tree/video test(s) failed\n";
        return 1;
    }
    std::cout << "All item/tree/video tests passed\n";
    return 0;
}
