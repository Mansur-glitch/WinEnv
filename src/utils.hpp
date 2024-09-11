#pragma once
#include "common.hpp"

#include <windows.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

namespace winenv {
// Выбрасывается при неуспешных вызовах winapi
class WinError : public std::runtime_error {
public:
  static std::string get_as_string(long long int win_err_code);
  WinError(const std::string &str);
  WinError(const std::string &str, DWORD win_err_code);
  DWORD get_error_code() const noexcept;

private:
  DWORD m_err_code;
};

// Выбрасывается при неправильном формате пользовательского литерала
class WrongLiteralFormat : public std::runtime_error {
public:
  WrongLiteralFormat(const std::string &str);
};

// Копирует string_view в Си-строку без проверки выхода за границу.
// Необходима как константное выражение в Hotkey::to_cstring()
constexpr void copy_from_string_view(std::string_view src, char *dest) {
  for (size_t i = 0; i < src.size(); ++i) {
    dest[i] = src[i];
  }
}

// Источник: boost/container_hash/hash.hpp
template <class T>
inline void boost_hash_combine(std::size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Вспомогательная структура для работы метода bind_instance
template <class ClassTy, class ReturnTy, class... Args> struct BindInstanceSt {
  // Параметр шаблона - последовательность индексов для прохода по кортежу.
  // Получили количество индексов равное количеству аргументов через
  // std::index_sequence_for
  template <size_t... Inds>
  static std::function<ReturnTy(Args...)>
  bind(ClassTy *instance, ReturnTy (ClassTy::*method)(Args...),
       std::index_sequence<Inds...>) {
    // Ограничение на максимум 10 аргументов, не учитывая указатель this
    static constexpr std::tuple placeholders{
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
        std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
        std::placeholders::_7, std::placeholders::_8, std::placeholders::_9,
        std::placeholders::_10};
    // Непосредственно создание функционального объекта.
    // Первый аргумент привязывает экземпляр класса
    // Остальные аргументы метода подменяются placeholder'ами
    return std::bind(method, instance, std::get<Inds>(placeholders)...);
  }
};

// Создает функциональный объект из экземпляра класса и его метода.
// Пример. Создание обработчика:
// void handle_hk(const MSG& msg) {...} // метод класса
// EventHandler hk_handler = bind_instance(this, handle_hk);
template <class ClassTy, class ReturnTy, class... Args>
std::function<ReturnTy(Args...)>
bind_instance(ClassTy *instance, ReturnTy (ClassTy::*method)(Args...)) {
  return BindInstanceSt<ClassTy, ReturnTy, Args...>::bind(
      instance, method, std::index_sequence_for<Args...>{});
}

std::wstring widen_string(std::string_view narrow);
std::string narrow_string(std::wstring_view wide);

// Создает широкую строку, которую можно передать как параметр командной строки
// Размер преобразуемого типа данных должен быть кратным двум
template <class Ty>
typename std::enable_if<sizeof(Ty) % 2 != 1, std::wstring>::type
cmd_arg_from(const Ty &data) {
  // Диапазон символов [0, 32], не используемых в аргументах командной строки
  // 32 - символ пробела - первый печатный символ - разделяет аргументы
  // командной строки
  static constexpr wchar_t not_used_beg = L'\0';
  static constexpr wchar_t not_used_end = L'!'; // Символ следующий за пробелом
  auto sy_data = reinterpret_cast<const wchar_t *>(&data);
  std::wstring arg_str;
  // Объем данных может увеличится до 2-х раз
  arg_str.resize(sizeof(Ty) * 2);
  size_t j = 0;
  for (size_t i = 0; i < sizeof(Ty) / sizeof(wchar_t); ++i, ++j) {
    if (not_used_beg <= sy_data[i] && sy_data[i] <= not_used_end) {
      arg_str[j++] = not_used_end; // Отмечаем символ требующий раскодирования
      arg_str[j] = sy_data[i] + not_used_end; // Закодированный символ допустим
    } else {
      arg_str[j] = sy_data[i];
    }
  }
  arg_str.resize(j);
  return arg_str;
}

// Создает объект из аргумента командой строки, полученного методом
// cmd_arg_from. Вторым элементом возвращает позицию символа, на котором
// завершилось преобразование
template <class Ty>
std::pair<Ty, size_t> cmd_arg_to(std::wstring_view arg_str) {
  static constexpr wchar_t special_sy = L'!';
  Ty data;
  auto sy_data = reinterpret_cast<wchar_t *>(&data);
  size_t j = 0;
  for (size_t i = 0; arg_str[j] != L'\0' && arg_str[j] != L' ' &&
                     i < sizeof(Ty) / sizeof(wchar_t);
       ++i, ++j) {
    if (arg_str[j] == special_sy) {
      sy_data[i] = arg_str[++j] - special_sy;
    } else {
      sy_data[i] = arg_str[j];
    }
  }
  return {data, j};
}

// Записывает в си-строку Id процесса и через разделитель '_' число тактов
// процессора прошедших со старта системы. Ставит '\0' в конце
void get_unique_window_title(char *str, size_t size);

// Возвращает путь к текущему исполняемому файлу
Path get_programm_path();
// Возвращает путь к стандартной программе командной строки
Path get_cmd_path();

// Возвращает переменную среды
std::string get_env_variable(std::string_view env_name);
// Устанавливает переменную среды или создает новую
void set_env_variable(std::string_view name, std::string_view value);

void invoke_message_box(std::string_view message);
} // namespace winenv
