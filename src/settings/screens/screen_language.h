#pragma once

#include "../navigation/ui_router.h"
#include "../core/theme.h"

namespace settings {

/**
 * @brief Language settings screen
 * 
 * Features:
 * - Language selection (RU/EN)
 * - Time format selection (24h/12h)
 */
class LanguageScreen : public Screen {
public:
    LanguageScreen() = default;
    ~LanguageScreen() override = default;
    
    bool create(lv_obj_t* parent) override;
    void destroy() override;
    ScreenType getType() const override { return ScreenType::LANGUAGE; }

private:
    void createHeader();
    void createLanguageSection();
    void createFormatSection();
    
    static void onLanguageSelected(lv_event_t* e);
    static void onFormatToggle(lv_event_t* e);
    
    lv_obj_t* header_ = nullptr;
    lv_obj_t* lang_dropdown_ = nullptr;
    lv_obj_t* format_switch_ = nullptr;
    ThemeColors theme_;
};

} // namespace settings
