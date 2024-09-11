#pragma once
#include <windows.h>

#include <filesystem>
#include <ostream>
#include <random>

namespace winenv
{
extern std::ostream* g_logger;
extern std::mt19937 g_rand_gen;

using Path = std::filesystem::path;
constexpr size_t g_max_file_path = MAX_PATH;
// Сигнатура обработчика оконных событий windows
using WindowProcedure = LRESULT (HWND, UINT, WPARAM, LPARAM);
} // namespace winenv


