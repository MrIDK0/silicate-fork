#include "system.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <filesystem>
#include <slc/formats/v3/replay.hpp>
#include <vector>

#include "bot/bot.hpp"
#include "bot/updater.hpp"

using namespace geode::prelude;

void ReplaySystem::onReset(uint32_t newFrame) {
    if (Bot::get()->isRecording()) {
        m_actionAtom.clipActions(newFrame);

        m_inputIndex = m_actionAtom.length();
    } else {
        if (m_actionAtom.m_actions.empty()) {
            m_inputIndex = 0;
            return;
        }

        m_inputIndex =
            std::distance(m_actionAtom.m_actions.begin(),
                          std::find_if(m_actionAtom.m_actions.begin(),
                                       m_actionAtom.m_actions.end(),
                                       [newFrame](const auto i) -> bool {
                                           return i.m_frame >= newFrame;
                                       }));
    }
}

[[nodiscard]] const std::optional<slc::Action> ReplaySystem::getNextInput(
    uint32_t frame) {
    if (m_inputIndex >= m_actionAtom.length()) {
        return std::nullopt;
    }

    auto& input = m_actionAtom.m_actions.at(m_inputIndex);
    if (input.m_frame == frame) {
        m_inputIndex++;
        return input;
    }

    return std::nullopt;
}

uint64_t& ReplaySystem::getCurrentRandomState() {
    return *reinterpret_cast<uint64_t*>(geode::base::get() + 0x6c2e90);
}

uint64_t& ReplaySystem::getCurrentShakeState() {
    return this->m_shakeRandomState;
}

void ReplaySystem::onExit() { m_inputIndex = 0; }

using Replay = slc::v3::Replay<>;

void ReplaySystem::save(std::filesystem::path path, bool noOverwrite) {
    if (noOverwrite && std::filesystem::exists(path)) {
        geode::log::info("Not overwriting replay at {}", path);
        return;
    }

    Replay replay;
    std::stable_sort(
        m_actionAtom.m_actions.begin(), m_actionAtom.m_actions.end(),
        [](const auto& a, const auto& b) { return a.m_frame < b.m_frame; });

    uint64_t previousFrame = 0;
    for (auto& action : m_actionAtom.m_actions) {
        // recalculate delta for all actions
        action.recalculateDelta(previousFrame);
        previousFrame = action.m_frame;
    }

    replay.m_atoms.add(m_actionAtom);

    replay.m_meta.m_build = 81;
    replay.m_meta.m_seed = m_startingSeed;
    replay.m_meta.m_tps = Bot::get()->updater().m_tps->inner();

    std::ofstream fd(path, std::ios::binary);
    auto result = replay.write(fd);
    if (!result.has_value()) {
        geode::log::error("Failed to save replay: {}",
                          result.error().m_message);
        return;
    }

    geode::log::info("Successfully saved replay to {}", path);
}

void ReplaySystem::processSlc3(Replay& replay) {
    auto& atoms = replay.m_atoms.m_atoms;
    auto it = std::find_if(atoms.begin(), atoms.end(), [](auto& v) {
        return std::visit(
            [](auto& at) { return at.id == slc::v3::AtomId::Action; }, v);
    });

    auto atom = *it;
    auto& updater = Bot::get()->updater();
    m_actionAtom = std::get<slc::ActionAtom>(atom);
    m_startingSeed = replay.m_meta.m_seed;
    updater.m_tps->inner() = replay.m_meta.m_tps;
    updater.m_tps->notifyChange();
    Bot::get()->setMode(Bot::Mode::Playing);
}

void ReplaySystem::processSlc2(slc::v2::Replay<ReplayMeta>& replay) {
    uint64_t currentFrame = 0;
    auto& a = m_actionAtom;
    a.clear();
    for (const auto& input : replay.getInputs()) {
        if (input.m_button == slc::v2::Input::InputType::Skip) {
            currentFrame = input.m_frame;
            continue;
        }

        if (static_cast<int>(input.m_button) <
            static_cast<int>(slc::v2::Input::InputType::Restart)) {
            a.m_actions.push_back(slc::v3::Action(
                currentFrame, input.m_frame - currentFrame,
                static_cast<slc::Action::ActionType>(input.m_button),
                input.m_holding, input.m_player2));
            currentFrame = input.m_frame;
            continue;
        }

        if (static_cast<int>(input.m_button) <
            static_cast<int>(slc::v2::Input::InputType::TPS)) {
            a.m_actions.push_back(slc::v3::Action(
                currentFrame, input.m_frame - currentFrame,
                static_cast<slc::v3::Action::ActionType>(input.m_button),
                replay.m_meta.seed));
            currentFrame = input.m_frame;
            continue;
        }

        a.m_actions.push_back(slc::v3::Action(
            currentFrame, input.m_frame - currentFrame, input.m_tps));
        currentFrame = input.m_frame;
    }

    auto& updater = Bot::get()->updater();
    m_startingSeed = replay.m_meta.seed;
    updater.m_tps->inner() = replay.m_tps;
    updater.m_tps->notifyChange();
    Bot::get()->setMode(Bot::Mode::Playing);
}

void ReplaySystem::load(std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        geode::log::error(
            "Failed to load slc3 replay from {}; file does not exist", path);
        return;
    }

    std::ifstream fd(path, std::ios::binary);

    auto replay = Replay::read(fd);
    if (replay.has_value()) {
        geode::log::info("Loading slc3 replay from {}", path);
        this->processSlc3(replay.value());
    } else {
        fd.seekg(0, std::ios::beg);
        auto v2Replay = slc::v2::Replay<ReplayMeta>::read(fd);
        if (v2Replay.has_value()) {
            geode::log::info("Loading slc2 (legacy) replay from {}", path);
            this->processSlc2(v2Replay.value());
        } else {
            geode::log::error("Failed to load slc3 replay from {}", path);
        }
    }

    // Reset merge mode when loading full replay
    m_mergeMode = false;
    m_loadedPlayer1 = false;
    m_loadedPlayer2 = false;
}

void ReplaySystem::loadPlayer1(std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        geode::log::error(
            "Failed to load slc3 replay from {}; file does not exist", path);
        return;
    }

    std::ifstream fd(path, std::ios::binary);

    auto replay = Replay::read(fd);
    if (replay.has_value()) {
        geode::log::info("Loading player 1 from slc3 replay from {}", path);
        
        slc::ActionAtom tempAtom;
        auto& atoms = replay.value().m_atoms.m_atoms;
        auto it = std::find_if(atoms.begin(), atoms.end(), [](auto& v) {
            return std::visit(
                [](auto& at) { return at.id == slc::v3::AtomId::Action; }, v);
        });

        if (it != atoms.end()) {
            auto atom = *it;
            tempAtom = std::get<slc::ActionAtom>(atom);
        }

        // Filter actions for player 1 only (m_player2 == false)
        std::vector<slc::Action> player1Actions;
        for (const auto& action : tempAtom.m_actions) {
            if (!action.m_player2) {
                player1Actions.push_back(action);
            }
        }

        // If in merge mode and player 2 already loaded, merge the actions
        if (m_mergeMode && m_loadedPlayer2) {
            for (const auto& action : player1Actions) {
                m_actionAtom.m_actions.push_back(action);
            }
            // Sort actions by frame
            std::stable_sort(
                m_actionAtom.m_actions.begin(), m_actionAtom.m_actions.end(),
                [](const auto& a, const auto& b) { return a.m_frame < b.m_frame; });
            
            // Recalculate deltas
            uint64_t previousFrame = 0;
            for (auto& action : m_actionAtom.m_actions) {
                action.recalculateDelta(previousFrame);
                previousFrame = action.m_frame;
            }
            
            geode::log::info("Merged player 1 actions with existing player 2 actions");
        } else {
            // Load only player 1 actions
            m_actionAtom.m_actions = player1Actions;
            m_mergeMode = true;
            m_startingSeed = replay.value().m_meta.m_seed;
            auto& updater = Bot::get()->updater();
            updater.m_tps->inner() = replay.value().m_meta.m_tps;
            updater.m_tps->notifyChange();
            Bot::get()->setMode(Bot::Mode::Playing);
        }

        m_loadedPlayer1 = true;
    } else {
        fd.seekg(0, std::ios::beg);
        auto v2Replay = slc::v2::Replay<ReplayMeta>::read(fd);
        if (v2Replay.has_value()) {
            geode::log::info("Loading player 1 from slc2 (legacy) replay from {}", path);
            
            slc::ActionAtom tempAtom;
            uint64_t currentFrame = 0;
            
            for (const auto& input : v2Replay.value().getInputs()) {
                if (input.m_button == slc::v2::Input::InputType::Skip) {
                    currentFrame = input.m_frame;
                    continue;
                }

                if (static_cast<int>(input.m_button) <
                    static_cast<int>(slc::v2::Input::InputType::Restart)) {
                    if (!input.m_player2) {
                        tempAtom.m_actions.push_back(slc::v3::Action(
                            currentFrame, input.m_frame - currentFrame,
                            static_cast<slc::Action::ActionType>(input.m_button),
                            input.m_holding, input.m_player2));
                    }
                    currentFrame = input.m_frame;
                    continue;
                }

                if (static_cast<int>(input.m_button) <
                    static_cast<int>(slc::v2::Input::InputType::TPS)) {
                    tempAtom.m_actions.push_back(slc::v3::Action(
                        currentFrame, input.m_frame - currentFrame,
                        static_cast<slc::v3::Action::ActionType>(input.m_button),
                        v2Replay.value().m_meta.seed));
                    currentFrame = input.m_frame;
                    continue;
                }

                tempAtom.m_actions.push_back(slc::v3::Action(
                    currentFrame, input.m_frame - currentFrame, input.m_tps));
                currentFrame = input.m_frame;
            }

            // Filter for player 1 only
            std::vector<slc::Action> player1Actions;
            for (const auto& action : tempAtom.m_actions) {
                if (!action.m_player2) {
                    player1Actions.push_back(action);
                }
            }

            if (m_mergeMode && m_loadedPlayer2) {
                for (const auto& action : player1Actions) {
                    m_actionAtom.m_actions.push_back(action);
                }
                std::stable_sort(
                    m_actionAtom.m_actions.begin(), m_actionAtom.m_actions.end(),
                    [](const auto& a, const auto& b) { return a.m_frame < b.m_frame; });
                
                uint64_t previousFrame = 0;
                for (auto& action : m_actionAtom.m_actions) {
                    action.recalculateDelta(previousFrame);
                    previousFrame = action.m_frame;
                }
                
                geode::log::info("Merged player 1 actions with existing player 2 actions");
            } else {
                m_actionAtom.m_actions = player1Actions;
                m_mergeMode = true;
                m_startingSeed = v2Replay.value().m_meta.seed;
                auto& updater = Bot::get()->updater();
                updater.m_tps->inner() = v2Replay.value().m_tps;
                updater.m_tps->notifyChange();
                Bot::get()->setMode(Bot::Mode::Playing);
            }

            m_loadedPlayer1 = true;
        } else {
            geode::log::error("Failed to load slc3 replay from {}", path);
        }
    }
}

void ReplaySystem::loadPlayer2(std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        geode::log::error(
            "Failed to load slc3 replay from {}; file does not exist", path);
        return;
    }

    std::ifstream fd(path, std::ios::binary);

    auto replay = Replay::read(fd);
    if (replay.has_value()) {
        geode::log::info("Loading player 2 from slc3 replay from {}", path);
        
        slc::ActionAtom tempAtom;
        auto& atoms = replay.value().m_atoms.m_atoms;
        auto it = std::find_if(atoms.begin(), atoms.end(), [](auto& v) {
            return std::visit(
                [](auto& at) { return at.id == slc::v3::AtomId::Action; }, v);
        });

        if (it != atoms.end()) {
            auto atom = *it;
            tempAtom = std::get<slc::ActionAtom>(atom);
        }

        // Filter actions for player 2 only (m_player2 == true)
        std::vector<slc::Action> player2Actions;
        for (const auto& action : tempAtom.m_actions) {
            if (action.m_player2) {
                player2Actions.push_back(action);
            }
        }

        // If in merge mode and player 1 already loaded, merge the actions
        if (m_mergeMode && m_loadedPlayer1) {
            for (const auto& action : player2Actions) {
                m_actionAtom.m_actions.push_back(action);
            }
            // Sort actions by frame
            std::stable_sort(
                m_actionAtom.m_actions.begin(), m_actionAtom.m_actions.end(),
                [](const auto& a, const auto& b) { return a.m_frame < b.m_frame; });
            
            // Recalculate deltas
            uint64_t previousFrame = 0;
            for (auto& action : m_actionAtom.m_actions) {
                action.recalculateDelta(previousFrame);
                previousFrame = action.m_frame;
            }
            
            geode::log::info("Merged player 2 actions with existing player 1 actions");
        } else {
            // Load only player 2 actions
            m_actionAtom.m_actions = player2Actions;
            m_mergeMode = true;
            m_startingSeed = replay.value().m_meta.m_seed;
            auto& updater = Bot::get()->updater();
            updater.m_tps->inner() = replay.value().m_meta.m_tps;
            updater.m_tps->notifyChange();
            Bot::get()->setMode(Bot::Mode::Playing);
        }

        m_loadedPlayer2 = true;
    } else {
        fd.seekg(0, std::ios::beg);
        auto v2Replay = slc::v2::Replay<ReplayMeta>::read(fd);
        if (v2Replay.has_value()) {
            geode::log::info("Loading player 2 from slc2 (legacy) replay from {}", path);
            
            slc::ActionAtom tempAtom;
            uint64_t currentFrame = 0;
            
            for (const auto& input : v2Replay.value().getInputs()) {
                if (input.m_button == slc::v2::Input::InputType::Skip) {
                    currentFrame = input.m_frame;
                    continue;
                }

                if (static_cast<int>(input.m_button) <
                    static_cast<int>(slc::v2::Input::InputType::Restart)) {
                    if (input.m_player2) {
                        tempAtom.m_actions.push_back(slc::v3::Action(
                            currentFrame, input.m_frame - currentFrame,
                            static_cast<slc::Action::ActionType>(input.m_button),
                            input.m_holding, input.m_player2));
                    }
                    currentFrame = input.m_frame;
                    continue;
                }

                if (static_cast<int>(input.m_button) <
                    static_cast<int>(slc::v2::Input::InputType::TPS)) {
                    tempAtom.m_actions.push_back(slc::v3::Action(
                        currentFrame, input.m_frame - currentFrame,
                        static_cast<slc::v3::Action::ActionType>(input.m_button),
                        v2Replay.value().m_meta.seed));
                    currentFrame = input.m_frame;
                    continue;
                }

                tempAtom.m_actions.push_back(slc::v3::Action(
                    currentFrame, input.m_frame - currentFrame, input.m_tps));
                currentFrame = input.m_frame;
            }

            // Filter for player 2 only
            std::vector<slc::Action> player2Actions;
            for (const auto& action : tempAtom.m_actions) {
                if (action.m_player2) {
                    player2Actions.push_back(action);
                }
            }

            if (m_mergeMode && m_loadedPlayer1) {
                for (const auto& action : player2Actions) {
                    m_actionAtom.m_actions.push_back(action);
                }
                std::stable_sort(
                    m_actionAtom.m_actions.begin(), m_actionAtom.m_actions.end(),
                    [](const auto& a, const auto& b) { return a.m_frame < b.m_frame; });
                
                uint64_t previousFrame = 0;
                for (auto& action : m_actionAtom.m_actions) {
                    action.recalculateDelta(previousFrame);
                    previousFrame = action.m_frame;
                }
                
                geode::log::info("Merged player 2 actions with existing player 1 actions");
            } else {
                m_actionAtom.m_actions = player2Actions;
                m_mergeMode = true;
                m_startingSeed = v2Replay.value().m_meta.seed;
                auto& updater = Bot::get()->updater();
                updater.m_tps->inner() = v2Replay.value().m_tps;
                updater.m_tps->notifyChange();
                Bot::get()->setMode(Bot::Mode::Playing);
            }

            m_loadedPlayer2 = true;
        } else {
            geode::log::error("Failed to load slc3 replay from {}", path);
        }
    }
}

std::filesystem::path ReplaySystem::getCurrentPath() {
    return Mod::get()->getPersistentDir(true) / "replays" /
           (m_replayName + ".slc");
}

static std::filesystem::path createBackupPath(const std::string& name) {
    namespace time = std::chrono;

    const time::time_point timestamp =
        time::floor<time::milliseconds>(time::system_clock::now());

    const std::filesystem::path path =
        Mod::get()->getPersistentDir(true) / "backups" /
        fmt::format("_backup_{:%Y%m%d_%H%M%S}_{}.slc", timestamp, name);

    return path;
}

void ReplaySystem::backupExisting(std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::string name = path.stem().string();
    auto newPath = createBackupPath(name);
    std::filesystem::copy(path, newPath,
                          std::filesystem::copy_options::skip_existing);
}

void ReplaySystem::createBackup() {
    auto path = createBackupPath(m_replayName);
    this->save(path, true);
}

$execute {
    auto bot = Bot::get();
    auto& rs = bot->replaySystem();

    rs.m_autosaveId = bot->scheduler().schedule(
        [&rs]() {
            auto pl = PlayLayer::get();
            if (!pl) return;
            if (!rs.m_autosaveAtInterval->inner()) return;

            if (Bot::get()->isRecording()) {
                rs.createBackup();
            }
        },
        rs.m_autosaveInterval->inner(), true);

    rs.m_autosaveInterval->handle([bot, &rs](double& interval) {
        bot->scheduler().reschedule(rs.m_autosaveId, interval);
    });
}
