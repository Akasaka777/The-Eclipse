#include "Game/SaveData.h"

#include "Common/StringUtil.h"
#include "Game/GameContext.h"
#include "Common/MathUtil.h"
#include "Game/ItemDatabase.h"
#include "Game/QuestDatabase.h"
#include "Game/SwordSkill.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ecl {

namespace {

// セーブ形式のバージョン（構造を変えたら上げる）
constexpr int kSaveVersion = 1;

std::string g_lastError;

// "key value1 value2 ..." を分解する
std::vector<std::string> SplitTokens(const std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) tokens.push_back(token);
    return tokens;
}

int ToInt(const std::string& text, int fallback = 0)
{
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

float ToFloat(const std::string& text, float fallback = 0.0f)
{
    try {
        return std::stof(text);
    } catch (...) {
        return fallback;
    }
}

void EnsureDirectory()
{
#if defined(_WIN32)
    _mkdir(SaveSystem::DirectoryPath());
#else
    mkdir(SaveSystem::DirectoryPath(), 0755);
#endif
}

} // namespace

const char* SaveSystem::DirectoryPath()
{
    return "save";
}

const char* SaveSystem::FilePath()
{
    return "save/save.txt";
}

const std::string& SaveSystem::LastError()
{
    return g_lastError;
}

bool SaveSystem::Exists()
{
    std::ifstream file(FilePath());
    return file.good();
}

bool SaveSystem::Remove()
{
    if (std::remove(FilePath()) != 0) {
        g_lastError = "セーブデータを削除できませんでした";
        return false;
    }
    return true;
}

bool SaveSystem::Save(const GameContext& context)
{
    EnsureDirectory();

    std::ofstream file(FilePath());
    if (!file) {
        g_lastError = "セーブファイルを開けませんでした";
        return false;
    }

    const PlayerData& player = context.player;
    const Inventory& inventory = player.GetInventory();

    file << "# The Eclipse セーブデータ\n";
    file << "version " << kSaveVersion << "\n";

    // --- プレイヤー ---------------------------------------------------------
    file << "level " << player.Level() << "\n";
    file << "exp " << player.Exp() << "\n";
    file << "skillpoints " << player.SkillPoints() << "\n";
    file << "col " << inventory.Col() << "\n";
    file << "material " << inventory.Material() << "\n";

    // --- 所持品 -------------------------------------------------------------
    //   item <uid> <templateId> <rarity> <upgradeLevel> <攻撃> <防御> <HP> <MP>
    //        <クリ率> <クリ倍率> <MP回復> <移動> <攻撃速度>
    for (const EquipmentItem& item : inventory.Items()) {
        const Stats& base = item.baseStats;
        file << "item " << item.uid << ' ' << item.templateId << ' '
             << static_cast<int>(item.rarity) << ' ' << item.upgradeLevel << ' '
             << base.attack << ' ' << base.defense << ' ' << base.maxHp << ' ' << base.maxMp << ' '
             << base.critRate << ' ' << base.critDamage << ' ' << base.mpRegen << ' '
             << base.moveSpeed << ' ' << base.attackSpeed << "\n";
    }

    // --- 装備中 -------------------------------------------------------------
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        file << "equip " << i << ' ' << inventory.EquippedUid(static_cast<EquipSlot>(i)) << "\n";
    }

    // --- スキル -------------------------------------------------------------
    for (int i = 0; i < kSkillSlotCount; ++i) {
        file << "skillslot " << i << ' ' << player.SkillLoadout()[i] << "\n";
    }
    for (const SwordSkill& skill : SkillDatabase::Instance().AllSkills()) {
        if (player.IsSkillUnlocked(skill.id)) file << "unlocked " << skill.id << "\n";
    }

    // --- 進行状況 -----------------------------------------------------------
    for (const QuestDef& quest : QuestDatabase::Instance().Quests()) {
        if (player.IsQuestCleared(quest.id)) file << "cleared " << quest.id << "\n";
    }
    file << "selectedquest " << context.selectedQuestId << "\n";

    // --- 設定 ---------------------------------------------------------------
    const GameSettings& settings = context.settings;
    file << "bgm " << settings.bgmVolume << "\n";
    file << "se " << settings.seVolume << "\n";
    file << "damagenumbers " << (settings.showDamageNumbers ? 1 : 0) << "\n";
    file << "screenshake " << (settings.screenShake ? 1 : 0) << "\n";
    file << "showfps " << (settings.showFps ? 1 : 0) << "\n";
    file << "fullscreen " << (settings.fullScreen ? 1 : 0) << "\n";
    file << "debugmode " << (settings.debugMode ? 1 : 0) << "\n";

    g_lastError.clear();
    return true;
}

bool SaveSystem::Load(GameContext& context)
{
    std::ifstream file(FilePath());
    if (!file) {
        g_lastError = "セーブデータが見つかりません";
        return false;
    }

    PlayerData loaded;
    Inventory& inventory = loaded.GetInventory();
    inventory.Clear();

    int equippedUid[static_cast<int>(EquipSlot::Count)] = {};
    int skillSlots[kSkillSlotCount] = {};
    std::vector<int> unlocked;
    std::vector<int> cleared;
    int version = 0;
    int level = 1;
    int exp = 0;
    int skillPoints = 0;
    int col = 0;
    int material = 0;
    int selectedQuest = 1;
    GameSettings settings = context.settings;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        const std::vector<std::string> tokens = SplitTokens(line);
        if (tokens.empty()) continue;
        const std::string& key = tokens[0];

        auto arg = [&](size_t index) -> std::string {
            return (index < tokens.size()) ? tokens[index] : std::string();
        };

        if (key == "version") version = ToInt(arg(1));
        else if (key == "level") level = ToInt(arg(1), 1);
        else if (key == "exp") exp = ToInt(arg(1));
        else if (key == "skillpoints") skillPoints = ToInt(arg(1));
        else if (key == "col") col = ToInt(arg(1));
        else if (key == "material") material = ToInt(arg(1));
        else if (key == "selectedquest") selectedQuest = ToInt(arg(1), 1);
        else if (key == "equip") {
            const int slot = ToInt(arg(1), -1);
            if (slot >= 0 && slot < static_cast<int>(EquipSlot::Count)) {
                equippedUid[slot] = ToInt(arg(2));
            }
        } else if (key == "skillslot") {
            const int slot = ToInt(arg(1), -1);
            if (slot >= 0 && slot < kSkillSlotCount) skillSlots[slot] = ToInt(arg(2));
        } else if (key == "unlocked") {
            unlocked.push_back(ToInt(arg(1)));
        } else if (key == "cleared") {
            cleared.push_back(ToInt(arg(1)));
        } else if (key == "item") {
            if (tokens.size() < 13) continue;
            EquipmentItem item;
            const int templateId = ToInt(arg(2));
            const ItemTemplate* tmpl = ItemDatabase::Instance().Find(templateId);
            if (!tmpl) continue;   // 定義が無くなったアイテムは捨てる

            item.uid = ToInt(arg(1));
            item.templateId = templateId;
            item.name = tmpl->name;
            item.flavor = tmpl->flavor;
            item.slot = tmpl->slot;
            item.weaponType = tmpl->weaponType;
            item.skin = tmpl->skin;
            item.rarity = static_cast<Rarity>(
                math::ClampInt(ToInt(arg(3)), 0, static_cast<int>(Rarity::Count) - 1));
            item.upgradeLevel = ToInt(arg(4));

            item.baseStats.attack = ToFloat(arg(5));
            item.baseStats.defense = ToFloat(arg(6));
            item.baseStats.maxHp = ToFloat(arg(7));
            item.baseStats.maxMp = ToFloat(arg(8));
            item.baseStats.critRate = ToFloat(arg(9));
            item.baseStats.critDamage = ToFloat(arg(10));
            item.baseStats.mpRegen = ToFloat(arg(11));
            item.baseStats.moveSpeed = ToFloat(arg(12));
            item.baseStats.attackSpeed = (tokens.size() > 13) ? ToFloat(arg(13)) : 0.0f;

            // 高レアリティのスキン補正を再適用
            if (static_cast<int>(item.rarity) >= static_cast<int>(Rarity::SR)) {
                const ColorRGB rarityColor = RarityColor(item.rarity);
                const float blend = 0.25f + 0.18f * static_cast<float>(
                    static_cast<int>(item.rarity) - static_cast<int>(Rarity::SR));
                item.skin.glow = ColorRGB::Lerp(item.skin.glow, rarityColor, blend);
                item.skin.secondary = ColorRGB::Lerp(item.skin.secondary, rarityColor, blend * 0.6f);
            }
            inventory.AddItem(item);
        }
        else if (key == "bgm") settings.bgmVolume = ToInt(arg(1), settings.bgmVolume);
        else if (key == "se") settings.seVolume = ToInt(arg(1), settings.seVolume);
        else if (key == "damagenumbers") settings.showDamageNumbers = ToInt(arg(1), 1) != 0;
        else if (key == "screenshake") settings.screenShake = ToInt(arg(1), 1) != 0;
        else if (key == "showfps") settings.showFps = ToInt(arg(1), 0) != 0;
        else if (key == "fullscreen") settings.fullScreen = ToInt(arg(1), 0) != 0;
        else if (key == "debugmode") settings.debugMode = ToInt(arg(1), 0) != 0;
    }

    if (version <= 0) {
        g_lastError = "セーブデータの形式が不正です";
        return false;
    }

    // --- 復元 ---------------------------------------------------------------
    loaded.RestoreProgress(level, exp, skillPoints, unlocked, cleared, skillSlots);
    inventory.SetCurrency(col, material);
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        if (equippedUid[i] != 0) inventory.Equip(equippedUid[i]);
    }
    // 同じ uid が再発行されないようにする
    int maxUid = 0;
    for (const EquipmentItem& item : inventory.Items()) {
        if (item.uid > maxUid) maxUid = item.uid;
    }
    ReserveItemUid(maxUid + 1);

    context.player = loaded;
    context.settings = settings;
    context.selectedQuestId = selectedQuest;

    g_lastError.clear();
    return true;
}

} // namespace ecl
