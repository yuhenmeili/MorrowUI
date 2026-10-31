#include "scene/AnimationService.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace morrow::editor {
namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string formatNumber(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    return buffer;
}

/// 解析 "VectorN(a, b, ...)" / "Color(...)" / 数值 的分量为 float；纯数值返回单分量。
bool parseNumericComponents(const std::string& value, std::string& prefix, std::string& suffix, std::vector<float>& components) {
    components.clear();
    const auto open = value.find('(');
    if (open == std::string::npos) {
        // 纯数值
        try {
            size_t consumed = 0;
            const float number = std::stof(value, &consumed);
            if (consumed != value.size())
                return false;
            prefix.clear();
            suffix.clear();
            components.push_back(number);
            return true;
        } catch (...) {
            return false;
        }
    }
    if (value.empty() || value.back() != ')')
        return false;
    prefix = value.substr(0, open + 1);
    suffix = ")";
    std::istringstream stream(value.substr(open + 1, value.size() - open - 2));
    std::string component;
    while (std::getline(stream, component, ',')) {
        try {
            components.push_back(std::stof(trim(component)));
        } catch (...) {
            return false;
        }
    }
    return !components.empty();
}

/// 数值形态序列化值重组（保持原前缀/精度 3 位）。
std::string composeValue(const std::string& prefix, const std::string& suffix, const std::vector<float>& components) {
    if (prefix.empty())
        return formatNumber(components.front());
    std::ostringstream output;
    output << prefix;
    for (size_t index = 0; index < components.size(); ++index) {
        if (index > 0)
            output << ", ";
        output << formatNumber(components[index]);
    }
    output << suffix;
    return output.str();
}

}  // namespace

int AnimationTrack::keyIndexAt(float time) const {
    for (size_t index = 0; index < keys.size(); ++index) {
        if (std::abs(keys[index].first - time) < 1e-4f)
            return static_cast<int>(index);
    }
    return -1;
}

void AnimationTrack::setKey(float time, const std::string& value) {
    if (const int existing = keyIndexAt(time); existing >= 0) {
        keys[static_cast<size_t>(existing)].second = value;
        return;
    }
    keys.emplace_back(time, value);
    std::sort(keys.begin(), keys.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
}

bool AnimationClip::fromSubResource(const SceneSubResourceRecord& record, AnimationClip& clip, std::string& error) {
    AnimationClip parsed;
    try {
        if (const auto duration = record.properties.find("duration"); duration != record.properties.end())
            parsed.duration = std::stof(duration->second);
    } catch (...) {
        error = "animation duration must be a number";
        return false;
    }
    if (const auto loop = record.properties.find("loop"); loop != record.properties.end())
        parsed.loop = loop->second == "true";
    int trackCount = 0;
    if (const auto count = record.properties.find("track_count"); count != record.properties.end()) {
        try {
            trackCount = std::stoi(count->second);
        } catch (...) {
            error = "animation track_count must be an integer";
            return false;
        }
    }
    for (int index = 0; index < trackCount; ++index) {
        const std::string prefix = "track_" + std::to_string(index);
        const auto target = record.properties.find(prefix + "_target");
        const auto property = record.properties.find(prefix + "_property");
        const auto keys = record.properties.find(prefix + "_keys");
        if (target == record.properties.end() || property == record.properties.end() || keys == record.properties.end()) {
            error = "animation track " + std::to_string(index) + " is missing target/property/keys";
            return false;
        }
        AnimationTrack track;
        track.targetId = target->second;
        track.property = property->second;
        // keys: "t=v;t=v;..."
        std::istringstream stream(keys->second);
        std::string entry;
        while (std::getline(stream, entry, ';')) {
            const auto equals = entry.find('=');
            if (equals == std::string::npos) {
                error = "animation key must be time=value: " + entry;
                return false;
            }
            try {
                const float time = std::stof(trim(entry.substr(0, equals)));
                track.keys.emplace_back(time, trim(entry.substr(equals + 1)));
            } catch (...) {
                error = "animation key time must be numeric: " + entry;
                return false;
            }
        }
        if (track.keys.empty()) {
            error = "animation track " + std::to_string(index) + " has no keys";
            return false;
        }
        std::sort(track.keys.begin(), track.keys.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
        parsed.tracks.push_back(std::move(track));
    }
    clip = std::move(parsed);
    return true;
}

SceneSubResourceRecord AnimationClip::toSubResource(const std::string& id) const {
    SceneSubResourceRecord record;
    record.id = id;
    record.type = "Animation";
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.3f", duration);
    record.properties["duration"] = buffer;
    record.properties["loop"] = loop ? "true" : "false";
    record.properties["track_count"] = std::to_string(tracks.size());
    for (size_t index = 0; index < tracks.size(); ++index) {
        const std::string prefix = "track_" + std::to_string(index);
        record.properties[prefix + "_target"] = tracks[index].targetId;
        record.properties[prefix + "_property"] = tracks[index].property;
        std::ostringstream packed;
        for (size_t key = 0; key < tracks[index].keys.size(); ++key) {
            if (key > 0)
                packed << ';';
            packed << formatNumber(tracks[index].keys[key].first) << '=' << tracks[index].keys[key].second;
        }
        record.properties[prefix + "_keys"] = packed.str();
    }
    return record;
}

bool AnimationClip::sample(const AnimationTrack& track, float time, std::string& value) const {
    if (track.keys.empty())
        return false;
    if (time <= track.keys.front().first) {
        value = track.keys.front().second;
        return true;
    }
    if (time >= track.keys.back().first) {
        value = track.keys.back().second;
        return true;
    }
    for (size_t index = 1; index < track.keys.size(); ++index) {
        if (time <= track.keys[index].first) {
            const auto& [leftTime, leftValue] = track.keys[index - 1];
            const auto& [rightTime, rightValue] = track.keys[index];
            // 数值/向量分量线性插值；字符串值阶梯保持（取左值）
            std::string leftPrefix, leftSuffix, rightPrefix, rightSuffix;
            std::vector<float> leftComponents, rightComponents;
            if (parseNumericComponents(leftValue, leftPrefix, leftSuffix, leftComponents) &&
                parseNumericComponents(rightValue, rightPrefix, rightSuffix, rightComponents) && leftComponents.size() == rightComponents.size() &&
                leftPrefix == rightPrefix) {
                const float span = rightTime - leftTime;
                const float ratio = span > 0.0f ? (time - leftTime) / span : 0.0f;
                std::vector<float> blended(leftComponents.size());
                for (size_t component = 0; component < leftComponents.size(); ++component)
                    blended[component] = leftComponents[component] + (rightComponents[component] - leftComponents[component]) * ratio;
                value = composeValue(leftPrefix, leftSuffix, blended);
            } else {
                value = leftValue;
            }
            return true;
        }
    }
    value = track.keys.back().second;
    return true;
}

AnimationTrack& AnimationClip::trackFor(const std::string& targetId, const std::string& property) {
    for (auto& track : tracks) {
        if (track.targetId == targetId && track.property == property)
            return track;
    }
    tracks.push_back({targetId, property, {}});
    return tracks.back();
}

void AnimationService::reload(const SceneDocument& document) {
    std::map<std::string, AnimationPlayerState> next;
    for (const auto& node : document.nodes()) {
        if (node.type != "MRAnimationPlayer")
            continue;
        const auto animationId = node.properties.find("animation_id");
        if (animationId == node.properties.end() || animationId->second.empty())
            continue;
        const auto* subResource = document.findSubResource(animationId->second);
        if (!subResource || subResource->type != "Animation")
            continue;
        AnimationPlayerState player;
        player.nodeId = node.id;
        std::string error;
        if (!AnimationClip::fromSubResource(*subResource, player.clip, error))
            continue;
        if (const auto speed = node.properties.find("speed"); speed != node.properties.end()) {
            try {
                player.speed = std::stof(speed->second);
            } catch (...) {
            }
        }
        if (const auto autoEntry = node.properties.find("autoplay"); autoEntry != node.properties.end() && autoEntry->second == "true")
            player.playing = true;
        next.emplace(node.id, std::move(player));
    }
    // 保留既有播放进度（同名播放器）
    for (auto& [id, player] : next) {
        const auto existing = m_players.find(id);
        if (existing != m_players.end()) {
            player.time = existing->second.time;
            player.playing = existing->second.playing || player.playing;
        }
    }
    m_players = std::move(next);
}

void AnimationService::play(const std::string& nodeId) {
    if (auto* player = find(nodeId))
        player->playing = true;
}

void AnimationService::pause(const std::string& nodeId) {
    if (auto* player = find(nodeId))
        player->playing = false;
}

void AnimationService::stop(const std::string& nodeId) {
    if (auto* player = find(nodeId)) {
        player->playing = false;
        player->time = 0.0f;
    }
}

void AnimationService::previewAt(const std::string& nodeId, float time, const AnimationApplyValue& apply) {
    auto* player = find(nodeId);
    if (!player)
        return;
    player->time = time;
    applyPlayerAt(*player, time, apply);
}

void AnimationService::tick(const AnimationApplyValue& apply) {
    const auto now = std::chrono::steady_clock::now();
    float dt = 0.0f;
    if (m_lastTickValid)
        dt = std::chrono::duration<float>(now - m_lastTick).count();
    m_lastTick = now;
    m_lastTickValid = true;
    if (dt <= 0.0f || dt > 0.5f)
        dt = dt <= 0.0f ? 0.016f : 0.5f;

    for (auto& [id, player] : m_players) {
        if (!player.playing || player.clip.tracks.empty())
            continue;
        player.time += dt * player.speed;
        if (player.time > player.clip.duration) {
            player.time = player.clip.loop ? std::fmod(player.time, player.clip.duration) : player.clip.duration;
            if (!player.clip.loop)
                player.playing = false;
        }
        applyPlayerAt(player, player.time, apply);
    }
}

const AnimationPlayerState* AnimationService::find(const std::string& nodeId) const {
    const auto entry = m_players.find(nodeId);
    return entry == m_players.end() ? nullptr : &entry->second;
}

AnimationPlayerState* AnimationService::find(const std::string& nodeId) {
    const auto entry = m_players.find(nodeId);
    return entry == m_players.end() ? nullptr : &entry->second;
}

void AnimationService::updateClip(const std::string& nodeId, const AnimationClip& clip) {
    if (auto* player = find(nodeId))
        player->clip = clip;
}

void AnimationService::applyPlayerAt(AnimationPlayerState& player, float time, const AnimationApplyValue& apply) {
    if (!apply)
        return;
    for (const auto& track : player.clip.tracks) {
        std::string value;
        if (player.clip.sample(track, time, value))
            apply(track.targetId, track.property, value);
    }
}

}  // namespace morrow::editor
