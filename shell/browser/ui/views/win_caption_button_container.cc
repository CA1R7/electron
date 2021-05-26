// Copyright (c) 2021 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_caption_button_container.h"

#include <memory>

#include "shell/browser/ui/views/win_caption_button.h"
#include "shell/browser/ui/views/win_frame_view.h"

#include "ui/base/l10n/l10n_util.h"
//#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

namespace electron {

namespace {

std::unique_ptr<WinCaptionButton> CreateCaptionButton(
    views::Button::PressedCallback callback,
    WinFrameView* frame_view,
    ViewID button_type,
    int accessible_name_resource_id) {
  return std::make_unique<WinCaptionButton>(
      std::move(callback), frame_view, button_type,
      l10n_util::GetStringUTF16(accessible_name_resource_id));
}

bool HitTestCaptionButton(WinCaptionButton* button, const gfx::Point& point) {
  return button && button->GetVisible() && button->bounds().Contains(point);
}

}  // anonymous namespace

WinCaptionButtonContainer::WinCaptionButtonContainer(WinFrameView* frame_view)
    : frame_view_(frame_view),
      minimize_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Minimize,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_MINIMIZE_BUTTON,
          IDS_APP_ACCNAME_MINIMIZE))),
      maximize_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Maximize,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_MAXIMIZE_BUTTON,
          IDS_APP_ACCNAME_MAXIMIZE))),
      restore_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Restore,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_RESTORE_BUTTON,
          IDS_APP_ACCNAME_RESTORE))),
      close_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::CloseWithReason,
                              base::Unretained(frame_view_->frame()),
                              views::Widget::ClosedReason::kCloseButtonClicked),
          frame_view_,
          VIEW_ID_CLOSE_BUTTON,
          IDS_APP_ACCNAME_CLOSE))) {
  // Layout is horizontal, with buttons placed at the trailing end of the view.
  // This allows the container to expand to become a faux titlebar/drag handle.
  auto* const layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kEnd)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStart)
      .SetDefault(
          views::kFlexBehaviorKey,
          views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                                   views::MinimumFlexSizeRule::kPreferred,
                                   views::MaximumFlexSizeRule::kPreferred,
                                   /* adjust_width_for_height */ false,
                                   views::MinimumFlexSizeRule::kScaleToZero));
}

WinCaptionButtonContainer::~WinCaptionButtonContainer() {}

// TODO: Confirm that this NonClientHitTest is able to be called properly (right
// now it isnt being called by anything)
int WinCaptionButtonContainer::NonClientHitTest(const gfx::Point& point) const {
  // FIXME(@mlaurencin): This DCHECK needs to pass, but I am commenting now to
  // view pointer value
  DCHECK(HitTestPoint(point))
      << "should only be called with a point inside this view's bounds";
  // LOG(INFO)
  //     << "WinCaptionButtonContainer::NonClientHitTest - Button frame_view - "
  //     << static_cast<void*>(frame_view_) << " - " << __LINE__;
  if (HitTestCaptionButton(minimize_button_, point)) {
    auto name = minimize_button_->GetAccessibleName();
    LOG(INFO) << "WinCaptionButtonContainer::NonClientHitTest - Minimize "
                 "button captioned - "
              << name << " - " << __LINE__;
    return HTMINBUTTON;
  }
  if (HitTestCaptionButton(maximize_button_, point)) {
    LOG(INFO) << "WinCaptionButtonContainer::NonClientHitTest - Maximize "
                 "button captioned - "
              << __LINE__;
    return HTMAXBUTTON;
  }
  if (HitTestCaptionButton(restore_button_, point)) {
    LOG(INFO) << "WinCaptionButtonContainer::NonClientHitTest - Restore "
                 "button captioned - "
              << __LINE__;
    return HTMAXBUTTON;
  }
  if (HitTestCaptionButton(close_button_, point)) {
    LOG(INFO) << "WinCaptionButtonContainer::NonClientHitTest - Close "
                 "button captioned - "
              << __LINE__;
    return HTCLOSE;
  }
  return HTCAPTION;
}

void WinCaptionButtonContainer::ResetWindowControls() {
  minimize_button_->SetState(views::Button::STATE_NORMAL);
  maximize_button_->SetState(views::Button::STATE_NORMAL);
  restore_button_->SetState(views::Button::STATE_NORMAL);
  close_button_->SetState(views::Button::STATE_NORMAL);
  InvalidateLayout();
}

void WinCaptionButtonContainer::AddedToWidget() {
  views::Widget* const widget = GetWidget();

  DCHECK(!widget_observation_.IsObserving());
  widget_observation_.Observe(widget);

  UpdateButtons();

  if (frame_view_->window()->IsWindowControlsOverlayEnabled()) {
    // SetBackground(
    //     views::CreateSolidBackground(frame_view_->GetTitlebarColor()));
    // BrowserView paints to a layer, so this must do the same to ensure that it
    // paints on top of the BrowserView.
    SetPaintToLayer();
  }
}

void WinCaptionButtonContainer::RemovedFromWidget() {
  DCHECK(widget_observation_.IsObserving());
  widget_observation_.Reset();
}

void WinCaptionButtonContainer::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  UpdateButtons();
}

void WinCaptionButtonContainer::UpdateButtons() {
  const bool is_maximized = frame_view_->frame()->IsMaximized();
  restore_button_->SetVisible(is_maximized);
  maximize_button_->SetVisible(!is_maximized);

  // In touch mode, windows cannot be taken out of fullscreen or tiled mode, so
  // the maximize/restore button should be disabled.
  const bool is_touch = ui::TouchUiController::Get()->touch_ui();
  restore_button_->SetEnabled(!is_touch);
  maximize_button_->SetEnabled(!is_touch);
  InvalidateLayout();
}
}  // namespace electron