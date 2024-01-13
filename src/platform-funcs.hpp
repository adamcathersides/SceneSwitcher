#pragma once
#include <vector>
#include <string>
#include <QStringList>
#include <chrono>
#include <optional>

namespace advss {

enum class HotkeyType;

// TODO: Implement for MacOS and Linux
extern std::chrono::high_resolution_clock::time_point lastMouseLeftClickTime;
extern std::chrono::high_resolution_clock::time_point lastMouseMiddleClickTime;
extern std::chrono::high_resolution_clock::time_point lastMouseRightClickTime;

void GetWindowList(std::vector<std::string> &windows);
void GetWindowList(QStringList &windows);
void GetCurrentWindowTitle(std::string &title);
#ifdef _WIN32
std::string GetWindowClassByWindowTitle(const std::string &window);
#endif
bool IsFullscreen(const std::string &title);
bool IsMaximized(const std::string &title);
std::optional<std::string> GetTextInWindow(const std::string &window);
int SecondsSinceLastInput();
void GetProcessList(QStringList &processes);
void GetForegroundProcessName(std::string &name);
bool IsInFocus(const QString &executable);
void PressKeys(const std::vector<HotkeyType> keys, int duration);
void PlatformInit();
void PlatformCleanup();

} // namespace advss
