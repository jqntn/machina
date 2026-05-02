#include <raylib.h>
#include <raymath.h>

// ==============
// === DRIVER ===
// ==============

extern "C"
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
  __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance =
    0x00000001;
}

// =============
// === UTILS ===
// =============

static void
draw_fps()
{
  DrawText(TextFormat("%d", GetFPS()), 8, 4, 30, GREEN);
}

// ==================
// === ENTRYPOINT ===
// ==================

int
main()
{
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
  InitWindow(1, 1, "machina");

  int current_monitor = GetCurrentMonitor();
  int monitor_width = GetMonitorWidth(current_monitor);
  int monitor_height = GetMonitorHeight(current_monitor);
  SetWindowSize(monitor_width + 1, monitor_height + 1);
  SetWindowPosition(0, 0);

  InitAudioDevice();
  SetExitKey(KEY_NULL);

  bool show_fps = false;

  Model model = LoadModel("../../../assets/models/suzannes.glb");
  Vector3 position = Vector3Zeros;
  Camera camera = {
    .position = Vector3Scale(Vector3Ones, 5.0f),
    .target = position,
    .up = Vector3UnitY,
    .fovy = 60.0f,
    .projection = CAMERA_PERSPECTIVE,
  };

  while (!WindowShouldClose()) {
    show_fps ^= IsKeyPressed(KEY_F1);

    UpdateCamera(&camera, CAMERA_ORBITAL);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);
    DrawModel(model, position, 1.0f, WHITE);
    DrawGrid(10, 1.0f);
    EndMode3D();

    if (show_fps) {
      draw_fps();
    }

    EndDrawing();
  }

  CloseAudioDevice();
  CloseWindow();
}