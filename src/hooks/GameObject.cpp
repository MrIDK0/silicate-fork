#include <Geode/Geode.hpp>
using namespace geode::prelude;
#include <Geode/modify/CCNode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "bot/bot.hpp"
#include "bot/updater.hpp"

static const std::unordered_set<int> OTHER_DECO_IDS = {
    902,  943,  944,  945,  946,  947,  948,  949,  950,  951,
    980,  981,  982,  983,  984,  985,  986,  987,  988,  1023,
    1024, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1032, 1063,
    1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071};

// Full trigger ID list from Eclipse | Also added some new ones thats why 1916 and 3643 are out of places
static const std::unordered_set<int> TRIGGER_IDS = {
    22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,
    55,   56,   57,   58,   59,   105,  744,  899,  900,  901,  915,  1006,
    1007, 1049, 1268, 1346, 1347, 1520, 1585, 1595, 1611, 1612, 1613, 1616,
    1811, 1812, 1814, 1815, 1816, 1817, 1818, 1819, 1912, 1913, 1914, 1915, 1916,
    1917, 1931, 1932, 1934, 1935, 2015, 2016, 2062, 2066, 2067, 2068, 2899,
    2900, 2901, 2904, 2905, 2907, 2909, 2910, 2911, 2912, 2913, 2914, 2915,
    2916, 2917, 2919, 2920, 2921, 2922, 2923, 2924, 2925, 2999, 3016, 3017,
    3018, 3019, 3020, 3021, 3022, 3023, 3024, 3029, 3030, 3031, 3033, 3600,
    3602, 3603, 3604, 3605, 3606, 3607, 3608, 3609, 3612, 3613, 3614, 3615,
    3617, 3618, 3619, 3620, 3640, 3641, 3642, 3643, 3655, 3660, 3661, 3662, 3643,
    3006, 3007, 3008, 3009, 3010, 3011, 3012, 3013, 3014, 3015, 2903};

// Helper function to check if an object is a trigger
static bool isTriggerObject(GameObject* obj) {
    if (!obj) return false;
    return TRIGGER_IDS.contains(obj->m_objectID);
}

// Silicate logic should still be there because well why not lmao im not removing it its still useful
struct SLGameObject : Modify<SLGameObject, GameObject> {
    bool isNoHitboxObject() {
        auto rect = this->getObjectRect();
        return rect.getMinX() == rect.getMaxX() &&
               rect.getMinY() == rect.getMaxY();
    }

    void setVisible(bool visible) {
        if (!LevelEditorLayer::get() &&
            Bot::get()->updater().m_layoutMode->inner()) {
            bool showTextAndCounters =
                Bot::get()->updater().m_layoutModeText->inner();

            if (showTextAndCounters &&
                (m_objectID == 914 || m_objectID == 1615)) {
                m_isHide = false;
                GameObject::setVisible(true);
                return;
            }
        }
        GameObject::setVisible(visible);
    }

    void addGlow(gd::string p0) {
        GameObject::addGlow(std::move(p0));

        if (LevelEditorLayer::get()) return;

        if (Bot::get()->updater().m_layoutMode->inner()) {
            bool showTextAndCounters =
                Bot::get()->updater().m_layoutModeText->inner();

            if (showTextAndCounters &&
                (m_objectID == 914 || m_objectID == 1615)) {
                m_isHide = false;
                m_isDontFade = true;
                m_isDontEnter = true;
            } else if (m_objectType == GameObjectType::Decoration ||
                       m_isNoTouch ||
                       (m_objectID >= 506 && m_objectID <= 640) ||
                       OTHER_DECO_IDS.contains(m_objectID)) {
                m_isHide = true;
            } else {
                m_isHide = false;
                m_isDontFade = true;
                m_isDontEnter = true;

                if (m_isPassable) {
                    m_opacityMod = 0.5;
                    m_opacityMod2 = 0.5;
                }
            }
        }
    }
};

// Eclipse's Show Triggers implementation
static GameObject* s_lastEditedTrigger = nullptr;
static int s_originalSectionIndex = -1;

class $modify(ShowTriggersPlayLayer, PlayLayer) {
    void addObject(GameObject* obj) {
        if (!obj) {
            PlayLayer::addObject(obj);
            return;
        }

        // Check if Show All Triggers is enabled
        bool showAllTriggers =
            Bot::get()->updater().m_layoutModeAllTriggers->inner();

        if (showAllTriggers && isTriggerObject(obj)) {
            // This is the exact logic from Eclipse
            bool idsCheck = obj->m_objectID == 3613 || obj->m_objectID == 3662;
            if ((obj->m_classType != GameObjectClassType::Effect ||
                 !static_cast<EffectGameObject*>(obj)->m_isTouchTriggered) &&
                obj->m_objectID != 2063) {
                s_lastEditedTrigger = obj;
                s_originalSectionIndex = obj->m_outerSectionIndex;
                // Set outer section to -1 so it gets processed properly
                obj->m_outerSectionIndex = -1;
            }
        }

        PlayLayer::addObject(obj);
    }
};

class $modify(ShowTriggersGJBase, GJBaseGameLayer) {
    void addToGroups(GameObject* obj, bool p1) {
        // Manually add the trigger to section and restore the original section
        // index
        if (obj == s_lastEditedTrigger) {
            obj->m_outerSectionIndex = s_originalSectionIndex;
            GJBaseGameLayer::addToSection(obj);
            s_lastEditedTrigger = nullptr;
        }
        GJBaseGameLayer::addToGroups(obj, p1);
    }
};

// Also need to modify GameObject::customSetup like Eclipse does
class $modify(ShowTriggersGameObject, GameObject) {
    void customSetup() override {
        bool editorEnabled = this->m_editorEnabled;
        int id = this->m_objectID;

        // Check if Show All Triggers is enabled and this is a trigger
        bool showAllTriggers =
            Bot::get()->updater().m_layoutModeAllTriggers->inner();

        if (showAllTriggers && TRIGGER_IDS.contains(id)) {
            // Temporarily enable editor mode so trigger shows up
            this->m_editorEnabled = true;
        }

        GameObject::customSetup();

        // Restore original editorEnabled state
        this->m_editorEnabled = editorEnabled;
    }
};