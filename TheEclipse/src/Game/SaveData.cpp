#include "Game/SaveData.h"

#include "Common/StringUtil.h"
#include "Game/Ability.h"
#include "Game/GameContext.h"
#include "Common/MathUtil.h"
#include "Game/ItemDatabase.h"
#include "Game/QuestDatabase.h"
#include "Game/SwordSkill.h"
#include "Game/UniqueSkill.h"

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
// v3: 武器スロットを左右に分割し、ユニークスキルを追加
// v8: 振り分けステータス（STR / AGI / VIT / INT / LUK）と MP 回復量の見直し
constexpr int kSaveVersion = 8;

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
    // セーブデータを削除した後は書き戻さない（削除が次回起動まで残るようにする）
    if (!context.autoSaveEnabled) {
        g_lastError = "セーブデータを削除したため保存しません";
        return false;
    }

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
    file << "name " << player.Name() << "\n";
    file << "level " << player.Level() << "\n";
    file << "exp " << player.Exp() << "\n";
    file << "skillpoints " << player.SkillPoints() << "\n";

    // --- 振り分けステータス ---------------------------------------------------
    file << "abilitypoints " << player.AbilityPoints() << "\n";
    for (int i = 0; i < kAbilityCount; ++i) {
        file << "ability " << i << ' '
             << player.Abilities().values[i] << "\n";
    }
    file << "col " << inventory.Col() << "\n";
    file << "material " << inventory.Material() << "\n";

    // --- 所持品 -------------------------------------------------------------
    //   item <uid> <templateId> <個体値> <upgradeLevel> <攻撃> <防御> <HP> <MP>
    //        <クリ率> <クリ倍率> <MP回復> <移動> <攻撃速度> <耐久力>
    //        <強化の伸び:攻撃> <強化の伸び:クリ率> <強化の伸び:クリ倍率>
    for (const EquipmentItem& item : inventory.Items()) {
        const Stats& base = item.baseStats;
        file << "item " << item.uid << ' ' << item.templateId << ' '
             << item.iv << ' ' << item.upgradeLevel << ' '
             << base.attack << ' ' << base.defense << ' ' << base.maxHp << ' ' << base.maxMp << ' '
             << base.critRate << ' ' << base.critDamage << ' ' << base.mpRegen << ' '
             << base.moveSpeed << ' ' << base.attackSpeed << ' ' << item.durability << ' '
             << item.growthAttack << ' ' << item.growthCritRate << ' '
             << item.growthCritDamage << "\n";
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

    // --- ユニークスキル -------------------------------------------------------
    file << "uniqueskill " << static_cast<int>(player.UniqueSkill()) << "\n";
    file << "parrycount " << player.ParrySuccessCount() << "\n";
    for (int value : player.AvailableUniqueSkills()) {
        file << "uniqueavailable " << value << "\n";
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
    std::vector<int> uniqueAvailable;
    UniqueSkillType uniqueSkill = UniqueSkillType::None;
    int version = 0;
    int level = 1;
    int exp = 0;
    int skillPoints = 0;
    int col = 0;
    int material = 0;
    int selectedQuest = 1;
    std::string playerName;
    int parryCount = 0;
    AbilityScores abilities;
    int abilityPoints = 0;
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
        else if (key == "name") {
            // 名前は空白を含み得るので、キーの後ろをそのまま取り出す
            const size_t space = line.find(' ');
            if (space != std::string::npos) playerName = line.substr(space + 1);
        }
        else if (key == "level") level = ToInt(arg(1), 1);
        else if (key == "exp") exp = ToInt(arg(1));
        else if (key == "skillpoints") skillPoints = ToInt(arg(1));
        else if (key == "abilitypoints") abilityPoints = ToInt(arg(1));
        else if (key == "ability") {
            const int index = ToInt(arg(1), -1);
            if (index >= 0 && index < kAbilityCount) {
                abilities.Set(static_cast<Ability>(index), ToInt(arg(2), kAbilityInitialValue));
            }
        }
        else if (key == "col") col = ToInt(arg(1));
        else if (key == "material") material = ToInt(arg(1));
        else if (key == "selectedquest") selectedQuest = ToInt(arg(1), 1);
        else if (key == "equip") {
            int slot = ToInt(arg(1), -1);
            // v2 以前は武器スロットが 1 つだったため、以降のスロットを 1 つずらす
            if (version > 0 && version < 3 && slot > 0) slot += 1;
            if (slot >= 0 && slot < static_cast<int>(EquipSlot::Count)) {
                equippedUid[slot] = ToInt(arg(2));
            }
        } else if (key == "skillslot") {
            const int slot = ToInt(arg(1), -1);
            if (slot >= 0 && slot < kSkillSlotCount) skillSlots[slot] = ToInt(arg(2));
        } else if (key == "unlocked") {
            unlocked.push_back(ToInt(arg(1)));
        } else if (key == "parrycount") {
            parryCount = ToInt(arg(1));
        } else if (key == "uniqueskill") {
            const int value = ToInt(arg(1));
            if (value > 0 && value < static_cast<int>(UniqueSkillType::Count)) {
                uniqueSkill = static_cast<UniqueSkillType>(value);
            }
        } else if (key == "uniqueavailable") {
            const int value = ToInt(arg(1));
            if (value > 0 && value < static_cast<int>(UniqueSkillType::Count)) {
                uniqueAvailable.push_back(value);
            }
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
            // v4 以前はここがレアリティ（0〜4）だったので、個体値へ読み替える
            const int ivField = ToInt(arg(3));
            item.iv = (version > 0 && version < 5) ? ClampIv(ivField * 20 + 10)
                                                   : ClampIv(ivField);
            item.upgradeLevel = ToInt(arg(4));

            item.baseStats.attack = ToFloat(arg(5));
            item.baseStats.defense = ToFloat(arg(6));
            item.baseStats.maxHp = ToFloat(arg(7));
            item.baseStats.maxMp = ToFloat(arg(8));
            item.baseStats.critRate = ToFloat(arg(9));
            item.baseStats.critDamage = ToFloat(arg(10));
            item.baseStats.mpRegen = ToFloat(arg(11));
            // v7 以前は MP 回復量の桁が違うので、定義し直した値へ読み替える
            if (version > 0 && version < 8) item.baseStats.mpRegen = tmpl->base.mpRegen;
            item.baseStats.moveSpeed = ToFloat(arg(12));
            item.baseStats.attackSpeed = (tokens.size() > 13) ? ToFloat(arg(13)) : 0.0f;

            // 耐久力（旧バージョンのセーブには含まれないため満タン扱い）
            if (tokens.size() > 14) {
                item.durability = ToFloat(arg(14), item.MaxDurability());
            } else {
                item.RestoreDurability();
            }
            // 壊れない装備は耐久力 0 のまま残る。それ以外は最低 1 を残す。
            item.indestructible = tmpl->indestructible;
            if (!item.indestructible && item.durability <= 0.0f) item.durability = 1.0f;

            // 強化で伸びた割合（v6 以前には無いので、旧仕様の +9%/段 を復元する）
            if (tokens.size() > 17) {
                item.growthAttack = ToFloat(arg(15));
                item.growthCritRate = ToFloat(arg(16));
                item.growthCritDamage = ToFloat(arg(17));
            } else {
                const float legacy = 0.09f * static_cast<float>(item.upgradeLevel);
                item.growthAttack = legacy;
                item.growthCritRate = legacy;
                item.growthCritDamage = legacy;
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
    // v3 以前には名前が無いので、その場合は既定値のままにする
    if (!playerName.empty()) loaded.SetName(playerName);
    loaded.RestoreProgress(level, exp, skillPoints, unlocked, cleared, skillSlots,
                           uniqueSkill, uniqueAvailable, parryCount);
    // v7 以前にはステータスが無いので、レベルぶんのポイントを未割り振りで配る
    if (version > 0 && version < 8) {
        abilities = AbilityScores();
        abilityPoints = (loaded.Level() - 1) * kAbilityPointsPerLevel;
    }
    loaded.RestoreAbilities(abilities, abilityPoints);
    inventory.SetCurrency(col, material);
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        if (equippedUid[i] == 0) continue;
        inventory.EquipTo(equippedUid[i], static_cast<EquipSlot>(i));
    }
    // 装備を戻してからスキル構成を整える
    //   （使えるスキルの系統は装備で決まるため、装備より先に整えると外れてしまう）
    loaded.RestoreSkillLoadout(skillSlots);

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
