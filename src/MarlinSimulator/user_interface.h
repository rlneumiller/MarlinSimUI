#pragma once

#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>

#include <gl.h>
#include <imgui.h>
#include "../ImGuiFileDialog/ImGuiFileDialog.h"

#include "execution_control.h"

/**
 * |-----|-----|------|
 * |Stat | LCD |      |
 * |           |      |
 * |           |  3D  |
 * |-----------|      |
 * |Ser        |      |
 * |-----------|------|
 */

static constexpr const char* ImGuiDefaultLayout =
R"(
[Window][DockSpaceWindwow]
Pos=0,0
Size=1280,720
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Components]
Pos=1003,0
Size=277,720
Collapsed=0
DockId=0x00000006,0

[Window][Simulation]
Pos=0,0
Size=313,107
Collapsed=0
DockId=0x00000003,0

[Window][Viewport]
Pos=315,0
Size=686,482
Collapsed=0
DockId=0x00000009,0

[Window][Status]
Pos=0,109
Size=313,611
Collapsed=0
DockId=0x00000004,0

[Window][Signal Analyser]
Pos=315,0
Size=686,482
Collapsed=0
DockId=0x00000009,1

[Window][Pin List]
Pos=1003,0
Size=277,720
Collapsed=0
DockId=0x00000006,1

[Window][Serial Monitor(0)]
Pos=315,484
Size=686,236
Collapsed=0
DockId=0x0000000A,0

[Window][Serial Monitor(1)]
Pos=315,484
Size=686,236
Collapsed=0
DockId=0x0000000A,1

[Window][Serial Monitor(2)]
Pos=315,484
Size=686,236
Collapsed=0
DockId=0x0000000A,2

[Window][Serial Monitor(3)]
Pos=315,484
Size=686,236
Collapsed=0
DockId=0x0000000A,3

[Window][Serial Monitor]
Pos=272,453
Size=623,267
Collapsed=0
DockId=0x00000008,0

[Window][Choose File##ChooseFileDlgKey]
Pos=271,95
Size=678,431
Collapsed=0

[Table][0x5E7B4F09,4]
RefScale=13
Column 0  Sort=0v
Column 1
Column 2
Column 3

[Docking][Data]
DockSpace         ID=0x6F13380E Window=0x49B6D357 Pos=0,0 Size=1280,720 Split=X
  DockNode        ID=0x00000005 Parent=0x6F13380E SizeRef=1001,720 Split=X
    DockNode      ID=0x00000001 Parent=0x00000005 SizeRef=313,720 Split=Y Selected=0x7CAC602A
      DockNode    ID=0x00000003 Parent=0x00000001 SizeRef=637,107 Selected=0x848745AB
      DockNode    ID=0x00000004 Parent=0x00000001 SizeRef=637,611 Selected=0x7CAC602A
    DockNode      ID=0x00000002 Parent=0x00000005 SizeRef=686,720 Split=Y Selected=0x995B0CF8
      DockNode    ID=0x00000007 Parent=0x00000002 SizeRef=448,451 Split=Y Selected=0x995B0CF8
        DockNode  ID=0x00000009 Parent=0x00000007 SizeRef=623,482 CentralNode=1 Selected=0x995B0CF8
        DockNode  ID=0x0000000A Parent=0x00000007 SizeRef=623,236 Selected=0xB516B7B1
      DockNode    ID=0x00000008 Parent=0x00000002 SizeRef=448,267 Selected=0xB42549D5
  DockNode        ID=0x00000006 Parent=0x6F13380E SizeRef=277,720 Selected=0xA115F62D


)";

class UiWindow {
public:

  template<class... Args>
  UiWindow(std::string name, Args... args) : name(name), show_callback(args...) {}
  virtual void show() {
    //if (!active) return;
    if (!ImGui::Begin((char*)name.c_str())) { //, &active)) {
      ImGui::End();
      return;
    }
    if (show_callback) show_callback(this);
    ImGui::End();
  }
  std::string name;
  //bool active = true;
  std::function<void(UiWindow*)> show_callback;
};

class UserInterface {
public:
  UserInterface();
  ~UserInterface();

  template <typename T, class... Args>
  std::shared_ptr<T> addElement(std::string id, Args&&... args) {
    if (ui_elements.find(id) != ui_elements.end()) throw std::runtime_error("UI IDs must be unique"); //todo: gracefulness
    auto element = std::make_shared<T>(id, args...);
    ui_elements[id] = element;
    return element;
  }

  void show();
  void render();

  //std::vector<std::shared_ptr<UiWindow>> ui_elements;
  static std::map<std::string, std::shared_ptr<UiWindow>> ui_elements;
};

struct StatusWindow : public UiWindow {
  template<class... Args>
  StatusWindow( std::string name, ImVec4* clear_color, Args... args) : UiWindow{name, args...}, clear_color{clear_color} {}
  void show() override {
    if (!ImGui::Begin((char*)name.c_str())) {
      ImGui::End();
      return;
    }

    if (show_callback) show_callback(this);

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)clear_color);  // Edit 3 floats representing a color

    if (ImGui::Button("Button")) counter++;                 // Buttons return true when clicked (most widgets return true when edited/activated)

    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImVec4* clear_color = nullptr;
  float f = 0.0f;
  int counter = 0;
};

#include <serial.h>
extern MSerialT serial_stream_0;
extern MSerialT serial_stream_1;
extern MSerialT serial_stream_2;
extern MSerialT serial_stream_3;

struct SerialMonitor : public UiWindow {
  SerialMonitor(std::string name, MSerialT& serial_stream) : UiWindow(name), serial_stream(serial_stream) {};
  char InputBuf[256] = {};
  std::string working_buffer;
  struct line_meta {
    std::size_t count = {};
    std::string text = {};
  };
  std::deque<line_meta> line_buffer{};
  std::deque<std::string> command_history{};
  std::size_t history_index = 0;
  std::string input_buffer = {};
  bool scroll_follow = true;
  uint8_t scroll_follow_state = false;
  bool streaming = false;

  MSerialT& serial_stream;
  std::ifstream input_file;

  int input_callback(ImGuiInputTextCallbackData* data) {
    switch (data->EventFlag) {
      case ImGuiInputTextFlags_CallbackCompletion:   // TODO: just search history?
        break;

      case ImGuiInputTextFlags_CallbackHistory: {
        std::size_t history_index_prev = history_index;

        if (data->EventKey == ImGuiKey_UpArrow) {
          if (history_index < command_history.size()) history_index ++;
        }
        else if (data->EventKey == ImGuiKey_DownArrow) {
          if (history_index > 0) history_index--;
        }

        if (history_index > 0 && history_index != history_index_prev) {
          if (history_index_prev == 0)
            input_buffer = std::string{data->Buf};

          data->DeleteChars(0, data->BufTextLen);
          data->InsertChars(0, (char *)command_history[history_index - 1].c_str());
        }
        else if (history_index == 0) {
          data->DeleteChars(0, data->BufTextLen);
          data->InsertChars(0, (char *)input_buffer.c_str());
        }
      } break;
    }
    return 0;
  }

  void insert_text(std::string text) {
    working_buffer.append(text);
    std::size_t index = working_buffer.find('\n');
    while (index != std::string::npos) {
      if (line_buffer.size() == 0 || line_buffer.back().text != working_buffer.substr(0, index)) {
        line_buffer.push_back({1, working_buffer.substr(0, index)});
      } else line_buffer.back().count++;
      working_buffer = working_buffer.substr(index + 1);
      index = working_buffer.find('\n');
    }
  }

  void show() {
      // File read into serial port
    if (input_file.is_open() && serial_stream.receive_buffer.free() && streaming) {
      uint8_t buffer[HalSerial::receive_buffer_size]{};
      auto count = input_file.readsome((char*)buffer, serial_stream.receive_buffer.free());
      serial_stream.receive_buffer.write(buffer, count);
      if (count == 0) input_file.close();
    }

    if (!ImGui::Begin((char *)name.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
      ImGui::End();
      return;
    }

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Stream")) {
        if (ImGui::MenuItem("Select GCode File")) {
          ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", "GCode(*.gcode *.gc *.g){.gcode,.gc,.g},.*", ".");
        }
        if (input_file.is_open() && streaming)
          if (ImGui::MenuItem("Pause")) {
            streaming = false;
          }
        if (input_file.is_open() && !streaming) {
          if (ImGui::MenuItem("Resume")) {
            streaming = true;
          }
        }
        if (input_file.is_open()) {
          if (ImGui::MenuItem("Cancel")) {
            input_file.close();
            streaming = false;
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Copy Buffer")) {}
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoDocking))  {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        printf("Streaming file: %s\n", filePathName.c_str());
        input_file.open(filePathName);
        streaming = true;
      } else {
        printf("Bad selection");
      }
      ImGuiFileDialog::Instance()->Close();
    }

    ImGui::BeginGroup();
    const ImGuiWindowFlags child_flags = 0;
    const ImGuiID child_id = ImGui::GetID((void*)(intptr_t)0);
    auto size = ImGui::GetContentRegionAvail();
    size.y -= 25; // TODO: there must be a better way to fill 2 items on a line
    if (ImGui::BeginChild(child_id, size, true, child_flags)) {
      for (auto line : line_buffer) {
        if (line.count > 1) ImGui::TextWrapped("[%ld] %s", line.count, (char *)line.text.c_str());
        else  ImGui::TextWrapped("%s", (char *)line.text.c_str());
      }
      ImGui::TextWrapped("%s", (char *)working_buffer.c_str());

      // Automatically set follow when scrolled to max
      if (ImGui::GetScrollY() != ImGui::GetScrollMaxY() || scroll_follow_state == 2) scroll_follow = false;
      else scroll_follow = true;

      // Automatically rescroll to max when follow clicked
      if (scroll_follow || scroll_follow_state == 1) {
        scroll_follow_state = 0;
        ImGui::SetScrollHereY(1.0f);
      }
    }
    ImGui::EndChild();
    ImGui::EndGroup();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    ImGui::PushItemWidth(-27);
    if (ImGui::InputText("##SerialInput", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, [](ImGuiInputTextCallbackData* data) -> int { return ((SerialMonitor*)(data->UserData))->input_callback(data); }, (void*)this)) {
        std::string input(InputBuf);
        input.erase(std::find_if(input.rbegin(), input.rend(), [](int ch) { return !std::isspace(ch); }).base(), input.end()); //trim whitespace
        if (input.size()) {
          if (command_history.size() == 0 || command_history.front() != input) command_history.push_front(input);
          history_index = 0;
          input.push_back('\n');
          std::size_t count = serial_stream.receive_buffer.free();
          serial_stream.receive_buffer.write((uint8_t *)input.c_str(), std::min({count, input.size()}));
        }
        strcpy((char*)InputBuf, "");
        reclaim_focus = true;
    }
    ImGui::PopItemWidth();

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    ImGui::SameLine();

    bool last_follow_state = scroll_follow;
    if (ImGui::Checkbox("##scroll_follow", &scroll_follow)) {
      if (last_follow_state != scroll_follow) {
        scroll_follow_state = last_follow_state == false ? 1 : 2;
      } else {
        scroll_follow_state = 0;
      }
    }
    ImGui::End();
  }

};

struct TextureWindow : public UiWindow {
  bool hovered = false;
  bool focused = false;
  GLuint texture_id = 0;
  float aspect_ratio = 0.0f;

  template<class... Args>
  TextureWindow(std::string name, GLuint texture_id, float aratio, Args... args) : UiWindow(name, args...), texture_id{texture_id}, aspect_ratio(aratio) {}
  void show() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{2, 2});
    ImGui::Begin((char *)name.c_str(), nullptr);
    auto size = ImGui::GetContentRegionAvail();
    size.y = size.x / aspect_ratio;
    ImGui::Image((ImTextureID)(intptr_t)texture_id, size, ImVec2(0,0), ImVec2(1,1));
    hovered = ImGui::IsItemHovered();
    focused = ImGui::IsWindowFocused();
    if (show_callback) show_callback((UiWindow*)this);
    ImGui::End();
    ImGui::PopStyleVar();
  }
};
