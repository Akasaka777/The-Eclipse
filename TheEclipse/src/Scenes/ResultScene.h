//==============================================================================
// ResultScene.h : クエスト終了後のリザルト
//==============================================================================
#pragma once

#include "Core/Scene.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {

class ResultScene : public Scene
{
public:
    ResultScene();

    void OnEnter(GameContext& context) override;
    void Update(float dt, GameContext& context, SceneManager& manager) override;
    void Draw(GameContext& context) override;

    // --- 検証用 ---------------------------------------------------------------
    // 開放・習得のお知らせを画面中央に出しているか
    bool AnnouncementShown() const;
    int  AnnouncementCount() const { return static_cast<int>(announcements_.size()); }
    const Rect& AnnouncementCardRect() const { return announceCard_; }
    const Rect& AnnouncementButtonRect() const { return announceButton_.GetRect(); }
    const Rect& HomeButtonRect() const { return homeButton_.GetRect(); }

private:
    //--------------------------------------------------------------------------
    // 開放・習得のお知らせ
    //   ユニークスキルの解放など、新しく使えるようになったものがあるときは
    //   画面を薄暗くして中央に 1 件ずつ出す（確認するまでホームへは戻れない）。
    //--------------------------------------------------------------------------
    struct Announcement
    {
        std::string tag;                   // 「ユニークスキル開放」
        std::string name;                  // 開放したものの名前
        std::string description;
        std::vector<std::string> details;  // 効果（1 行ずつ）
        std::string condition;             // 達成した条件
        std::string guide;                 // 使えるようにするまでの手順
        ColorRGB    color = ColorRGB(255, 220, 120);
    };

    void BuildAnnouncements(const GameContext& context);
    void DrawHeader(const GameContext& context) const;
    void DrawStats(const GameContext& context) const;
    void DrawRewards(const GameContext& context) const;
    void DrawAnnouncement() const;

    ui::Button homeButton_;
    float time_ = 0.0f;
    int   scroll_ = 0;

    std::vector<Announcement> announcements_;
    int   announceIndex_ = 0;      // 今出している件（全部見たら announcements_.size()）
    float announceTimer_ = 0.0f;   // 今の件を出してからの時間
    Rect  announceCard_;
    ui::Button announceButton_;
};

} // namespace ecl
