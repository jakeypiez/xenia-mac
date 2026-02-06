/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/xam/ui/friends_ui.h"

#include <chrono>
#include <regex>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/imgui/imgui.h"
#include "xenia/base/string.h"
#include "xenia/base/string_util.h"
#include "xenia/emulator.h"
#include "xenia/kernel/XLiveAPI.h"
#include "xenia/kernel/kernel.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/xam/user_profile.h"
#include "xenia/kernel/xam/xam_state.h"
#include "xenia/ui/imgui_drawer.h"
#include "xenia/ui/imgui_host_notification.h"

using namespace std::chrono_literals;

namespace xe {
namespace kernel {
namespace xam {
namespace ui {

// ---------------------------------------------------------------------------
// Helper: Draw a single friend entry
// ---------------------------------------------------------------------------
static bool DrawFriendContent(xe::ui::ImGuiDrawer* imgui_drawer,
                              UserProfile* profile,
                              FriendPresenceObjectJSON& presence,
                              uint64_t* removed_xuid) {
  const uint32_t user_index =
      kernel_state()->xam_state()->GetUserIndexAssignedToProfileFromXUID(
          profile->GetLogonXUID());

  const uint64_t friend_xuid = presence.XUID();
  const std::string friend_xuid_str = fmt::format("{:016X}", friend_xuid);
  bool are_friends = profile->IsFriend(friend_xuid);
  bool is_self = profile->GetOnlineXUID() == friend_xuid;

  const uint32_t title_id = presence.TitleIDValue();
  const uint32_t current_title = kernel_state()->title_id();
  const bool same_title = title_id && title_id == current_title;

  // --- Friend info ---
  ImGui::PushID(static_cast<int>(friend_xuid & 0xFFFFFFFF));

  // Gamertag + online indicator
  if (presence.State()) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::TextUnformatted(presence.Gamertag().c_str());
    ImGui::PopStyleColor();
  } else {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::TextUnformatted(presence.Gamertag().c_str());
    ImGui::PopStyleColor();
  }

  ImGui::TextDisabled("  %s", friend_xuid_str.c_str());

  if (title_id) {
    if (same_title) {
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f),
                         "  Playing same game");
    } else {
      ImGui::Text("  Title: %s", presence.TitleID().c_str());
    }
  }

  if (!presence.RichPresence().empty()) {
    std::string presence_str =
        xe::string_util::trim(xe::to_utf8(presence.RichPresence()));
    if (!presence_str.empty()) {
      // Replace newlines with commas for display
      presence_str =
          std::regex_replace(presence_str, std::regex("\\n"), ", ");
      ImGui::TextWrapped("  Status: %s", presence_str.c_str());
    }
  }

  ImGui::Spacing();

  // --- Buttons ---
  float btn_height = 25;
  float btn_width = (ImGui::GetContentRegionAvail().x * 0.5f) -
                    (ImGui::GetStyle().ItemSpacing.x * 0.5f);
  ImVec2 half_btn = ImVec2(btn_width, btn_height);

  if (!is_self) {
    // Join Session button
    ImGui::BeginDisabled(!presence.SessionID() || !same_title);
    if (ImGui::Button("Join Session", half_btn)) {
      // TODO: Implement session join via invite acceptance
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      if (!presence.SessionID()) {
        ImGui::SetTooltip("No active session");
      } else if (!same_title) {
        ImGui::SetTooltip("Playing a different game");
      } else {
        ImGui::SetTooltip("Join gaming session");
      }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Remove / Add friend button
    if (are_friends) {
      if (ImGui::Button("Remove", half_btn)) {
        if (profile->RemoveFriend(friend_xuid)) {
          if (removed_xuid) {
            *removed_xuid = friend_xuid;
          }
          XLiveAPI::RemoveFriend(friend_xuid);

          kernel_state()->BroadcastNotification(
              kXNotificationFriendsFriendRemoved, user_index);

          std::string description =
              !presence.Gamertag().empty() ? presence.Gamertag() : "Success";

          kernel_state()
              ->emulator()
              ->display_window()
              ->app_context()
              .CallInUIThread([imgui_drawer, description]() {
                new xe::ui::HostNotificationWindow(
                    imgui_drawer, "Removed Friend", description, 0);
              });
        }
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Remove Friend");
      }
    } else {
      if (ImGui::Button("Add", half_btn)) {
        bool added = profile->AddFriendFromXUID(friend_xuid);

        if (added) {
          XLiveAPI::AddFriend(friend_xuid);
          kernel_state()->BroadcastNotification(
              kXNotificationFriendsFriendAdded, user_index);
        }

        std::string description =
            !presence.Gamertag().empty() ? presence.Gamertag()
                                        : (added ? "Success" : "Failed!");

        kernel_state()
            ->emulator()
            ->display_window()
            ->app_context()
            .CallInUIThread([imgui_drawer, description, added]() {
              new xe::ui::HostNotificationWindow(
                  imgui_drawer, added ? "Added Friend" : "Error",
                  description, 0);
            });
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Add Friend");
      }
    }
  }

  // Context menu (right-click to copy)
  if (ImGui::BeginPopupContextItem("##FriendCtx")) {
    if (ImGui::BeginMenu("Copy")) {
      if (ImGui::MenuItem("Gamertag")) {
        ImGui::SetClipboardText(presence.Gamertag().c_str());
      }
      ImGui::Separator();
      if (ImGui::MenuItem("XUID")) {
        ImGui::SetClipboardText(friend_xuid_str.c_str());
      }
      ImGui::EndMenu();
    }
    ImGui::EndPopup();
  }

  ImGui::PopID();
  return true;
}

// ---------------------------------------------------------------------------
// Helper: Draw the "Add Friend" modal popup
// ---------------------------------------------------------------------------
static bool DrawAddFriend(xe::ui::ImGuiDrawer* imgui_drawer,
                          UserProfile* profile, AddFriendArgs& args) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 center = viewport->GetCenter();

  if (!args.add_friend_open) {
    args.add_friend_first_draw = false;
  }

  float btn_height = 25;

  ImGui::SetNextWindowContentSize(ImVec2(280, 0));
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Add Friend", &args.add_friend_open,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (!args.add_friend_context_open &&
        ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_GamepadFaceRight, false)) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::SetWindowFontScale(1.05f);

    ImVec2 btn_size = ImVec2(ImGui::GetContentRegionAvail().x, btn_height);

    uint32_t user_index =
        kernel_state()->xam_state()->GetUserIndexAssignedToProfileFromXUID(
            profile->GetLogonXUID());

    bool max_friends = profile->GetFriendsCount() >= X_ONLINE_MAX_FRIENDS;

    if (max_friends) {
      ImGui::Text("Max Friends Reached!");
      ImGui::Separator();
    } else if (args.are_friends) {
      ImGui::Text("Friend Added!");
      ImGui::Separator();
    }

    const std::string xuid_string = std::string(args.add_xuid_);
    uint64_t xuid = 0;

    if (xuid_string.length() == 16) {
      if (xuid_string.starts_with("0009")) {
        xuid = xe::string_util::from_string<uint64_t>(xuid_string, true);
        args.valid_xuid = IsOnlineXUID(xuid);
        args.are_friends = profile->IsFriend(xuid);
      }

      if (!args.valid_xuid) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 50, 50, 255));
        if (xuid_string.starts_with("E")) {
          ImGui::Text("This is an offline XUID!");
        } else {
          ImGui::Text("Invalid XUID!");
        }
        ImGui::PopStyleColor();
        ImGui::Separator();
      }
    } else {
      args.valid_xuid = false;
      args.are_friends = false;
    }

    const std::string friends_count =
        fmt::format("Friends: {}/100", profile->GetFriendsCount());
    ImGui::TextDisabled("%s", friends_count.c_str());

    ImGui::Spacing();
    ImGui::Text("Enter Online XUID:");

    if (!args.add_friend_first_draw && std::string(args.add_xuid_).empty()) {
      args.add_friend_first_draw = true;
      ImGui::SetKeyboardFocusHere();
    }

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##AddFriend", "0009XXXXXXXXXXXX", args.add_xuid_,
                             sizeof(args.add_xuid_),
                             ImGuiInputTextFlags_CharsHexadecimal |
                                 ImGuiInputTextFlags_CharsUppercase);
    ImGui::PopItemWidth();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("Right Click to Paste");
    }

    // Paste / Clear context menu
    if (ImGui::BeginPopupContextItem("##AddFriendContexts")) {
      args.add_friend_context_open = true;

      if (ImGui::MenuItem("Paste")) {
        const std::string clipboard = ImGui::GetClipboardText();
        if (!clipboard.empty()) {
          strncpy(args.add_xuid_, clipboard.c_str(),
                  sizeof(args.add_xuid_) - 1);
        }
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Clear")) {
        memset(args.add_xuid_, 0, sizeof(args.add_xuid_));
      }

      ImGui::EndPopup();
    } else {
      args.add_friend_context_open = false;
    }

    ImGui::BeginDisabled(!args.valid_xuid || args.are_friends || max_friends);
    if (ImGui::Button("Add", btn_size)) {
      bool added = profile->AddFriendFromXUID(xuid);

      if (added) {
        XLiveAPI::AddFriend(xuid);
        args.added_friend = true;

        kernel_state()->BroadcastNotification(kXNotificationFriendsFriendAdded,
                                              user_index);
      }

      std::string desc = added ? xuid_string : "Failed!";

      kernel_state()
          ->emulator()
          ->display_window()
          ->app_context()
          .CallInUIThread([imgui_drawer, desc]() {
            new xe::ui::HostNotificationWindow(imgui_drawer, "Added Friend",
                                               desc, 0);
          });
    }
    ImGui::EndDisabled();

    ImGui::EndPopup();
  }

  return true;
}

// ---------------------------------------------------------------------------
// Helper: Draw the main friends list content
// ---------------------------------------------------------------------------
static bool DrawFriendsContent(
    xe::ui::ImGuiDrawer* imgui_drawer, UserProfile* profile,
    FriendsContentArgs& args,
    std::vector<FriendPresenceObjectJSON>* presences, bool loading) {
  if (!profile || !presences) {
    return false;
  }

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 center = viewport->GetCenter();

  ImGui::SetNextWindowSizeConstraints(ImVec2(420, 220), ImVec2(420, 600));
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(
          "Friends", &args.friends_open,
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
              ImGuiWindowFlags_AlwaysVerticalScrollbar)) {

    // Close on Gamepad B
    if (!args.add_friend_args.add_friend_open &&
        !args.add_friend_args.search_filter_context_open &&
        ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_GamepadFaceRight, false)) {
      ImGui::CloseCurrentPopup();
    }

    const float window_width = ImGui::GetContentRegionAvail().x;
    float btn_height = 25;
    float btn_width =
        (window_width * 0.5f) - (ImGui::GetStyle().ItemSpacing.x * 0.5f);
    ImVec2 half_width_btn = ImVec2(btn_width, btn_height);

    // --- Friends count ---
    const std::string friends_count =
        fmt::format("{}/100", profile->GetFriendsCount());
    ImGui::SetCursorPosX(window_width -
                         ImGui::CalcTextSize(friends_count.c_str()).x +
                         ImGui::GetStyle().WindowPadding.x);
    ImGui::TextDisabled("%s", friends_count.c_str());

    // --- Search bar ---
    if (args.first_draw) {
      args.first_draw = false;
      ImGui::SetKeyboardFocusHere();
    }

    args.filter.Draw("##Search", window_width);

    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Search by gamertag or XUID. Right-click to paste.");
    }

    // Search bar context menu
    if (ImGui::BeginPopupContextItem("##SearchFilter")) {
      args.add_friend_args.search_filter_context_open = true;

      if (ImGui::MenuItem("Paste")) {
        const std::string clipboard = ImGui::GetClipboardText();
        if (!clipboard.empty()) {
          memset(args.filter.InputBuf, 0, sizeof(args.filter.InputBuf));
          strncpy(args.filter.InputBuf, clipboard.c_str(),
                  sizeof(args.filter.InputBuf) - 1);
          args.filter.Build();
        }
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Clear")) {
        memset(args.filter.InputBuf, 0, sizeof(args.filter.InputBuf));
        args.filter.Build();
      }

      ImGui::EndPopup();
    } else {
      args.add_friend_args.search_filter_context_open = false;
    }

    // --- Filters ---
    ImGui::Checkbox("Joinable", &args.filter_joinable);
    ImGui::SameLine();
    ImGui::Checkbox("Same Game", &args.filter_title);
    ImGui::SameLine();
    ImGui::Checkbox("Hide Offline", &args.filter_offline);

    ImGui::Spacing();

    // --- Action buttons ---
    if (ImGui::Button("Add Friend",
                      ImVec2(ImGui::GetContentRegionAvail().x, btn_height))) {
      args.add_friend_args.add_friend_open = true;
      ImGui::OpenPopup("Add Friend");
    }

    ImGui::BeginDisabled(!profile->GetFriendsCount() || loading);
    if (ImGui::Button(loading ? "Refreshing..." : "Refresh", half_width_btn)) {
      args.refresh_presence = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!profile->GetFriendsCount());
    if (ImGui::Button("Remove All", half_width_btn)) {
      ImGui::OpenPopup("Remove All Friends");
    }
    ImGui::EndDisabled();

    // Add Friend popup
    DrawAddFriend(imgui_drawer, profile, args.add_friend_args);

    if (args.add_friend_args.added_friend) {
      args.refresh_presence = true;
      args.add_friend_args.added_friend = false;
    }

    ImGui::Separator();
    ImGui::Spacing();

    // --- Loading indicator ---
    if (loading) {
      ImGui::Spacing();
      const char* msg = "Loading friends...";
      ImVec2 msg_size = ImGui::CalcTextSize(msg);
      ImGui::SetCursorPosX(
          (ImGui::GetWindowWidth() - msg_size.x) * 0.5f);
      ImGui::TextDisabled("%s", msg);
      ImGui::Spacing();
    }

    // --- Friends list ---
    uint32_t visible_count = 0;
    for (uint32_t index = 0; auto& presence : *presences) {
      bool filter_gamertags =
          args.filter.PassFilter(presence.Gamertag().c_str());
      bool filter_xuid = args.filter.PassFilter(
          fmt::format("{:016X}",
                      static_cast<uint64_t>(presence.XUID()))
              .c_str());

      if (filter_gamertags || filter_xuid) {
        // Skip ourselves
        if (profile->GetOnlineXUID() ==
            static_cast<uint64_t>(presence.XUID())) {
          index++;
          continue;
        }

        const bool same_title =
            presence.TitleIDValue() &&
            presence.TitleIDValue() == kernel_state()->title_id();

        if (args.filter_joinable &&
            (!presence.SessionID() || !same_title)) {
          index++;
          continue;
        }

        if (args.filter_title && !same_title) {
          index++;
          continue;
        }

        if (args.filter_offline &&
            (!presence.State() ||
             !IsValidXUID(static_cast<uint64_t>(presence.XUID())))) {
          index++;
          continue;
        }

        uint64_t removed_xuid = 0;
        DrawFriendContent(imgui_drawer, profile, presence, &removed_xuid);

        if (removed_xuid) {
          presences->erase(presences->begin() + index);
          // Don't increment index since we erased
        } else {
          visible_count++;
          ImGui::Separator();
          ImGui::Spacing();
          index++;
        }
      } else {
        index++;
      }
    }

    // --- Empty state ---
    if (visible_count == 0 && presences->empty()) {
      ImGui::Spacing();
      const char* msg = "No friends yet. Use Add Friend to get started.";
      ImVec2 msg_size = ImGui::CalcTextSize(msg);
      ImGui::SetCursorPosX(
          (ImGui::GetWindowWidth() - msg_size.x) * 0.5f);
      ImGui::TextDisabled("%s", msg);
      ImGui::Spacing();
    } else if (visible_count == 0 && !presences->empty()) {
      ImGui::Spacing();
      const char* msg = "No friends match the current filters.";
      ImVec2 msg_size = ImGui::CalcTextSize(msg);
      ImGui::SetCursorPosX(
          (ImGui::GetWindowWidth() - msg_size.x) * 0.5f);
      ImGui::TextDisabled("%s", msg);
      ImGui::Spacing();
    }

    // --- Remove All confirmation ---
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(225, 90), ImVec2(225, 90));
    if (ImGui::BeginPopupModal("Remove All Friends", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      uint32_t user_index =
          kernel_state()->xam_state()->GetUserIndexAssignedToProfileFromXUID(
              profile->GetLogonXUID());

      float confirm_btn_width =
          (ImGui::GetContentRegionAvail().x * 0.5f) -
          (ImGui::GetStyle().ItemSpacing.x * 0.5f);
      ImVec2 btn_size = ImVec2(confirm_btn_width, btn_height);

      const std::string desc = "Are you sure?";
      ImVec2 desc_size = ImGui::CalcTextSize(desc.c_str());
      ImGui::SetCursorPosX((ImGui::GetWindowWidth() - desc_size.x) * 0.5f);
      ImGui::Text("%s", desc.c_str());
      ImGui::Separator();

      if (ImGui::Button("Yes", btn_size)) {
        profile->RemoveAllFriends();

        args.refresh_presence = true;

        kernel_state()->BroadcastNotification(
            kXNotificationFriendsFriendRemoved, user_index);

        kernel_state()
            ->emulator()
            ->display_window()
            ->app_context()
            .CallInUIThread([imgui_drawer]() {
              new xe::ui::HostNotificationWindow(
                  imgui_drawer, "Removed All Friends", "Success", 0);
            });

        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", btn_size)) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::EndPopup();
  }

  return true;
}

// ---------------------------------------------------------------------------
// FriendsUI dialog implementation
// ---------------------------------------------------------------------------

FriendsUI::FriendsUI(xe::ui::ImGuiDrawer* imgui_drawer, UserProfile* profile)
    : XamDialog(imgui_drawer), profile_(profile) {
  friends_presence_ = RefreshFriendsPresence(profile);
}

std::future<std::vector<FriendPresenceObjectJSON>>
FriendsUI::RefreshFriendsPresence(UserProfile* profile) {
  return std::async(std::launch::async, [profile]() {
    const uint8_t user_index =
        kernel_state()->xam_state()->GetUserIndexAssignedToProfileFromXUID(
            profile->xuid());

    return XLiveAPI::GetAllFriendsPresence(user_index);
  });
}

void FriendsUI::OnDraw(ImGuiIO& io) {
  if (!args.friends_open) {
    args.first_draw = true;
    args.friends_open = true;

    ImGui::OpenPopup("Friends");

    if (XLiveAPI::IsConnectedToServer()) {
      args.filter_offline = true;
    }
  }

  if (friends_presence_.valid()) {
    if (friends_presence_.wait_for(0s) == std::future_status::ready) {
      friends_presence_result_ = friends_presence_.get();
      loading_ = false;
    }
  }

  if (args.refresh_presence) {
    friends_presence_ = RefreshFriendsPresence(profile_);
    args.refresh_presence = false;
    loading_ = true;
  }

  DrawFriendsContent(imgui_drawer(), profile_, args,
                     &friends_presence_result_, loading_);

  if (!args.friends_open) {
    Close();
  }
}

}  // namespace ui
}  // namespace xam
}  // namespace kernel
}  // namespace xe
