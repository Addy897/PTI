#include "includes/pti_client.hpp"

#include "includes/raylib.h"
#include <filesystem>
#include <format>
#include <mutex>
#define RAYGUI_IMPLEMENTATION
#include "includes/raygui.h"

#include <string>
std::vector<std::string> logLines;
std::mutex logMutex;

void add_to_log(std::string msg) {
  std::lock_guard<std::mutex> lock(logMutex);
  logLines.push_back(msg);
}

void handler(const std::vector<std::string> &result, const std::string &id,
             const std::string &peer) {
  std::lock_guard<std::mutex> lock(logMutex);
  logLines.push_back(
      std::format("Got Intersection from {} for room: {}", peer, id));
  logLines.push_back(
      std::format("  Found {} common indicators.", result.size()));
  for (const auto &i : result) {
    logLines.push_back("    " + i);
  }
}
int main() {
  InitWindow(800, 600, "PTI GUI");
  SetTargetFPS(60);

  PTI pti("127.0.0.1");

  bool connected = false;
  pti.setResultHandler(handler);
  char joinRoomID[64] = "";
  int roomsScroll = 0;
  Rectangle panelRec = {50, 255, 700, 300};
  Rectangle panelContentRec = {0, 0, 700, 0};
  Rectangle panelView = {0, 0, 0, 0};
  Vector2 panelScroll = {0, 0};
  const int FONT_SIZE = 20;
  const int LINE_HEIGHT = FONT_SIZE + 2;
  bool show_error = false;
  std::string error;
  bool textBoxEditMode = false;
  bool showFileDialog = false;
  char indicatorFilePath[256] = "";
  bool join = false;
  float lineHeight = MeasureTextEx(GetFontDefault(), "A", FONT_SIZE, 1.0f).y;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (!connected) {
      if (GuiButton(Rectangle{50, 50, 120, 30}, "Connect")) {
        try {
          pti.start();
          connected = true;
          add_to_log("Connected to server.\n");
        } catch (std::runtime_error &e) {
          show_error = true;
          error = e.what();
        }
      }
    } else {
      GuiLabel(Rectangle{50, 50, 200, 30}, "Connected!");
    }

    if (GuiButton(Rectangle{50, 100, 120, 30}, "Get Rooms")) {
      std::string rooms = pti.getRooms();
      add_to_log("Rooms: " + rooms + "\n");
    }

    if (GuiButton(Rectangle{50, 150, 120, 30}, "Create Room")) {
      join = false;
      showFileDialog = true;
    }

    GuiTextBox(Rectangle{50, 200, 120, 30}, joinRoomID, 64, !textBoxEditMode);
    if (GuiButton(Rectangle{180, 200, 80, 30}, "Join")) {
      if (strlen(joinRoomID) > 0) {
        join = true;
        showFileDialog = true;
      } else {
        show_error = true;
        error = "Room ID Required!!";
      }
    }
    if (show_error) {
      int ret = GuiMessageBox(Rectangle{85, 70, 250, 100}, "Error",
                              error.c_str(), "OK");
      if (ret >= 0) {
        show_error = false;
        error = "";
      }
    }
    GuiGroupBox(Rectangle{50, 250, 700, 300}, "Logs");

    float contentHeight = lineHeight * logLines.size();
    if (contentHeight < panelRec.height)
      contentHeight = panelRec.height;
    panelContentRec.height = contentHeight;
    panelContentRec.width = panelRec.width;
    panelContentRec.x = 0;
    panelContentRec.y = 0;

    GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

    BeginScissorMode((int)panelRec.x, (int)panelRec.y, (int)panelRec.width,
                     (int)panelRec.height);
    {
      std::lock_guard<std::mutex> lock(logMutex);

      int firstLine = (int)(-panelScroll.y / lineHeight);
      int lastLine = firstLine + (int)(panelRec.height / lineHeight) + 2;
      if (firstLine < 0)
        firstLine = 0;
      if (lastLine > (int)logLines.size())
        lastLine = (int)logLines.size();

      for (int i = firstLine; i < lastLine; i++) {
        float posY = panelRec.y + panelScroll.y + (i * lineHeight);
        DrawText(logLines[i].c_str(), panelRec.x + 5, (int)posY, FONT_SIZE,
                 BLACK);
      }
    }
    EndScissorMode();

    panelContentRec.height = logLines.size() * lineHeight;
    if (showFileDialog) {
      Rectangle msgBoxRec = {200, 150, 400, 150};
      if (GuiWindowBox(msgBoxRec, "Indicator File")) {
        showFileDialog = false;
      }

      Rectangle textBoxRec = {msgBoxRec.x + 20, msgBoxRec.y + 50,
                              msgBoxRec.width - 40, 30};
      if (GuiTextBox(textBoxRec, indicatorFilePath, sizeof(indicatorFilePath),
                     textBoxEditMode)) {
        textBoxEditMode = !textBoxEditMode;
      }

      Rectangle okButtonRec = {msgBoxRec.x + (msgBoxRec.width / 2) - 50,
                               msgBoxRec.y + 100, 100, 30};
      if (GuiButton(okButtonRec, "OK")) {
        if (!std::filesystem::exists(indicatorFilePath)) {
          show_error = true;
          error = std::format("File {} does not exist", indicatorFilePath);
        } else {
          try {
            pti.loadIndicatorsFromFile(indicatorFilePath);
            if (join) {
              pti.joinRoom(joinRoomID);
              add_to_log("Joining Room: " + std::string(joinRoomID) + "\n");
              joinRoomID[0] = '\0';
            } else {
              std::string id = pti.createROOM();
              add_to_log("Created Room ID: " + id + "\n");
            }

          } catch (std::runtime_error &err) {
            error = err.what();
            show_error = true;
          }
        }

        memset(indicatorFilePath, 0, sizeof(indicatorFilePath));
        showFileDialog = false;
      }
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
