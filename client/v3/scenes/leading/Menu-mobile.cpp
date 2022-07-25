// Copyright 2021 <CatchChallenger>
#include "Menu.hpp"

using Scenes::Menu;

void Menu::OnScreenResize() {
  if (!is_loaded_) return;
  int buttonMargin = 10;
  buttonMargin = 30;
  solo_->SetSize(300, 120);
  solo_->SetPixelSize(45);
  multi_->SetSize(300, 120);
  multi_->SetPixelSize(45);
  options_->SetSize(90, 95);
  options_->SetPixelSize(45);
  debug_->SetSize(90, 95);
  debug_->SetPixelSize(45);
  facebook_->SetSize(90, 95);
  facebook_->SetPixelSize(45);
  website_->SetSize(90, 95);
  website_->SetPixelSize(45);
  int verticalMargin =
      BoundingRect().height() / 2 + multi_->Height() / 2 + buttonMargin;
  if (solo_->IsVisible()) {
    solo_->SetPos(BoundingRect().width() / 2 - solo_->Width() / 2,
                  BoundingRect().height() / 2 - multi_->Height() / 2 -
                      solo_->Height() - buttonMargin);
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height() / 2);
  } else {
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height());
    verticalMargin = BoundingRect().height() / 2 + buttonMargin;
  }
  debug_->SetPos(10, 10);
  const int horizontalMargin = (multi_->Width() - options_->Width() -
                                facebook_->Width() - website_->Width()) /
                               2;
  options_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2 -
                       horizontalMargin - options_->Width(),
                   verticalMargin);
  facebook_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2,
                    verticalMargin);
  website_->SetPos(
      BoundingRect().width() / 2 + facebook_->Width() / 2 + horizontalMargin,
      verticalMargin);
  warning_->SetPos(BoundingRect().width() / 2 - warning_->Width() / 2, 5);

  if (BoundingRect().height() < 280) {
    news_update_->SetVisible(false);
    if (current_news_type_ != 0) {
      current_news_type_ = 0;
      UpdateNews();
    }
  } else {
    uint8_t border_size = 46;
    news_list_->SetPos(10, 10);
    news_wait_->SetPos(news_->Width() - news_wait_->Width() - border_size - 2,
                       news_->Height() / 2 - news_wait_->Height() / 2);
    news_->SetVisible(true);

    if (BoundingRect().width() < 600 || BoundingRect().height() < 480) {
      int w = BoundingRect().width() - 9 * 2;
      if (w > 300) w = 300;
      news_->Strech(26, 26, 10, w, 40);
      news_wait_->SetVisible(BoundingRect().width() > 450 && !have_fresh_feed_);
      news_update_->SetVisible(false);

      if (current_news_type_ != 1) {
        current_news_type_ = 1;
        UpdateNews();
      }
    } else {
      news_->Strech(26, 26, 10, 800 - 9 * 2, 280);
      news_wait_->SetVisible(!have_fresh_feed_);
      news_update_->SetSize(136, 57);
      news_update_->SetPixelSize(18);
      news_update_->SetPos(news_->Width() - 136 - border_size - 2,
                           news_->Height() / 2 - 57 / 2);
      news_update_->SetVisible(have_update_);

      if (current_news_type_ != 2) {
        current_news_type_ = 2;
        UpdateNews();
      }
    }
    news_->SetPos(BoundingRect().width() / 2 - news_->Width() / 2,
                  BoundingRect().height() - news_->Height() - 5);
    news_list_->SetSize(news_->Width() - 20, news_->Height() - 20);
  }
}
