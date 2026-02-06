/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_XAM_UI_FRIENDS_UI_H_
#define XENIA_KERNEL_XAM_UI_FRIENDS_UI_H_

#include <future>
#include <vector>

#include "xenia/kernel/json/friend_presence_object_json.h"
#include "xenia/kernel/xam/ui/netplay_manager_util.h"
#include "xenia/kernel/xam/xam_ui.h"

namespace xe {
namespace kernel {
namespace xam {

class UserProfile;

namespace ui {

class FriendsUI : public XamDialog {
 public:
  FriendsUI(xe::ui::ImGuiDrawer* imgui_drawer, UserProfile* profile);

  std::future<std::vector<FriendPresenceObjectJSON>> RefreshFriendsPresence(
      UserProfile* profile);

 private:
  void OnDraw(ImGuiIO& io) override;

  UserProfile* profile_;
  FriendsContentArgs args = {};
  std::future<std::vector<FriendPresenceObjectJSON>> friends_presence_;
  std::vector<FriendPresenceObjectJSON> friends_presence_result_;
  bool loading_ = true;
};

}  // namespace ui
}  // namespace xam
}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_XAM_UI_FRIENDS_UI_H_
