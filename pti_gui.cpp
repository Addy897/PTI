#include "includes/pti_client.hpp"

#include "includes/raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "includes/raygui.h"

#include <string>

int main() {
  InitWindow(800, 600, "PTI GUI");
  SetTargetFPS(60);

  PTI pti("127.0.0.1");
  
  pti.loadIndicatorsFromFile("indicators.txt");

  bool connected = false;

  char joinRoomID[64] = "";
  std::string logText = "";
  int roomsScroll = 0;
  Rectangle panelRec = {50, 255, 700, 300};
  Rectangle panelContentRec = {0, 0, 700, 0};
  Rectangle panelView = {0, 0, 0, 0};
  Vector2 panelScroll = {0, 0};
  const int FONT_SIZE = 20;
  const int LINE_HEIGHT = FONT_SIZE + 2;
  bool show_error = false;
  std::string error;
  float lineHeight = MeasureTextEx(GetFontDefault(), "A", FONT_SIZE, 1.0f).y;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (!connected) {
      if (GuiButton(Rectangle{50, 50, 120, 30}, "Connect")) {
        try {
          pti.start();
          connected = true;
          logText += "Connected to server.\n";
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
      logText += "Rooms: " + rooms + "\n";
    }

    if (GuiButton(Rectangle{50, 150, 120, 30}, "Create Room")) {
      std::string id = pti.createROOM();
      logText += "Created Room ID: " + id + "\n";
    }

    GuiTextBox(Rectangle{50, 200, 120, 30}, joinRoomID, 64, true);
    if (GuiButton(Rectangle{180, 200, 80, 30}, "Join")) {
      if (strlen(joinRoomID) > 0) {
        pti.joinRoom(joinRoomID);
        logText += "Joining Room: " + std::string(joinRoomID) + "\n";
        joinRoomID[0] = '\0';
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
    std::vector<std::string> lines;
    {
      std::string tmp;
      for (char c : logText) {
        if (c == '\n') {
          lines.push_back(tmp);
          tmp.clear();
        } else {
          tmp.push_back(c);
        }
      }
      if (!tmp.empty())
        lines.push_back(tmp);
    }

    float contentHeight = lineHeight * lines.size();
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
      Vector2 pos = {panelRec.x + panelScroll.x, panelRec.y + panelScroll.y};
      for (size_t i = 0; i < lines.size(); i++) {
        DrawText(lines[i].c_str(), pos.x, pos.y + i * lineHeight, FONT_SIZE,
                 BLACK);
      }
    }
    EndScissorMode();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
