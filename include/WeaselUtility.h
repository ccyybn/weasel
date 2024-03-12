#pragma once
#include <filesystem>
#include <string>
#include <codecvt>
#include <set>
#include <list>
#include <map>
#include <bitset>
#include <sstream>
#include <psapi.h>

inline int utf8towcslen(const char* utf8_str, int utf8_len) {
  return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
}

inline std::wstring getUsername() {
  DWORD len = 0;
  GetUserName(NULL, &len);

  if (len <= 0) {
    return L"";
  }

  wchar_t* username = new wchar_t[len + 1];

  GetUserName(username, &len);
  if (len <= 0) {
    delete[] username;
    return L"";
  }
  auto res = std::wstring(username);
  delete[] username;
  return res;
}

// data directories
std::filesystem::path WeaselSharedDataPath();
std::filesystem::path WeaselUserDataPath();

inline BOOL IsUserDarkMode() {
  constexpr const LPCWSTR key =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  constexpr const LPCWSTR value = L"AppsUseLightTheme";

  DWORD type;
  DWORD data;
  DWORD size = sizeof(DWORD);
  LSTATUS st = RegGetValue(HKEY_CURRENT_USER, key, value, RRF_RT_REG_DWORD,
                           &type, &data, &size);

  if (st == ERROR_SUCCESS && type == REG_DWORD)
    return data == 0;
  return false;
}

inline std::wstring string_to_wstring(const std::string& str,
                                      int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return L"";
  // calc len
  int len =
      MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), NULL, 0);
  if (len <= 0)
    return L"";
  std::wstring res;
  TCHAR* buffer = new TCHAR[len + 1];
  MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), buffer, len);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}

inline std::string wstring_to_string(const std::wstring& wstr,
                                     int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return "";
  int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(),
                                NULL, 0, NULL, NULL);
  if (len <= 0)
    return "";
  std::string res;
  char* buffer = new char[len + 1];
  WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(), buffer, len,
                      NULL, NULL);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}

inline BOOL is_wow64() {
  DWORD errorCode;
  if (GetSystemWow64DirectoryW(NULL, 0) == 0)
    if ((errorCode = GetLastError()) == ERROR_CALL_NOT_IMPLEMENTED)
      return FALSE;
    else
      ExitProcess((UINT)errorCode);
  else
    return TRUE;
}

template <typename CharT>
struct EscapeChar {
  static const CharT escape;
  static const CharT linefeed;
  static const CharT tab;
  static const CharT linefeed_escape;
  static const CharT tab_escape;
};

template <>
const char EscapeChar<char>::escape = '\\';
template <>
const char EscapeChar<char>::linefeed = '\n';
template <>
const char EscapeChar<char>::tab = '\t';
template <>
const char EscapeChar<char>::linefeed_escape = 'n';
template <>
const char EscapeChar<char>::tab_escape = 't';

template <>
const wchar_t EscapeChar<wchar_t>::escape = L'\\';
template <>
const wchar_t EscapeChar<wchar_t>::linefeed = L'\n';
template <>
const wchar_t EscapeChar<wchar_t>::tab = L'\t';
template <>
const wchar_t EscapeChar<wchar_t>::linefeed_escape = L'n';
template <>
const wchar_t EscapeChar<wchar_t>::tab_escape = L't';

template <typename CharT>
inline std::basic_string<CharT> escape_string(
    const std::basic_string<CharT> input) {
  using Esc = EscapeChar<CharT>;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == Esc::escape) {
      res << Esc::escape << Esc::escape;
    } else if (*p == Esc::linefeed) {
      res << Esc::escape << Esc::linefeed_escape;
    } else if (*p == Esc::tab) {
      res << Esc::escape << Esc::tab_escape;
    } else {
      res << *p;
    }
  }
  return res.str();
}

template <typename CharT>
inline std::basic_string<CharT> unescape_string(
    const std::basic_string<CharT>& input) {
  using Esc = EscapeChar<CharT>;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == Esc::escape) {
      if (++p == input.end()) {
        break;
      } else if (*p == Esc::linefeed_escape) {
        res << Esc::linefeed;
      } else if (*p == Esc::tab_escape) {
        res << Esc::tab;
      } else {  // \a => a
        res << *p;
      }
    } else {
      res << *p;
    }
  }
  return res.str();
}

// resource
std::string GetCustomResource(const char* name, const char* type);

inline DWORD bit_set(DWORD number, DWORD n) {
  return number | ((DWORD)1 << n);
}

inline DWORD bit_unset(DWORD number, DWORD n) {
  return number & ~((DWORD)1 << n);
}

inline bool bit_check(DWORD number, DWORD n) {
  return (number >> n) & (DWORD)1;
}

inline std::wstring s2ws(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.from_bytes(str);
}

inline std::string ws2s(const std::wstring& wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes(wstr);
}

inline std::vector<std::string> split(std::string s,
                                      const std::string& delimiter) {
  std::vector<std::string> out;

  size_t pos = 0;
  std::string token;
  int i = 0;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    out.push_back(token);
    s.erase(0, pos + delimiter.length());
    i++;
  }
  out.push_back(s);
  return out;
}

inline std::vector<std::wstring> split(std::wstring s,
                                       const std::wstring& delimiter) {
  std::vector<std::wstring> out;

  size_t pos = 0;
  std::wstring token;
  int i = 0;
  while ((pos = s.find(delimiter)) != std::wstring::npos) {
    token = s.substr(0, pos);
    out.push_back(token);
    s.erase(0, pos + delimiter.length());
    i++;
  }
  out.push_back(s);
  return out;
}

inline std::set<std::string> split2set(std::string s,
                                       const std::string& delimiter) {
  std::set<std::string> out;

  size_t pos = 0;
  std::string token;
  int i = 0;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    out.insert(token);
    s.erase(0, pos + delimiter.length());
    i++;
  }
  out.insert(s);
  return out;
}

inline int indexOf(std::string s,
                   const std::string& item,
                   const std::string& delimiter) {
  size_t pos = 0;
  std::string token;
  int i = 0;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    if (!token.compare(item))
      return i;
    s.erase(0, pos + delimiter.length());
    i++;
  }
  if (!s.compare(item))
    return i;
  return -1;
}

inline int indexOf(std::wstring s,
                   const std::wstring& item,
                   const std::wstring& delimiter) {
  size_t pos = 0;
  std::wstring token;
  int i = 0;
  while ((pos = s.find(delimiter)) != std::wstring::npos) {
    token = s.substr(0, pos);
    if (!token.compare(item))
      return i;
    s.erase(0, pos + delimiter.length());
    i++;
  }
  if (!s.compare(item))
    return i;
  return -1;
}

inline int indexOf(std::list<std::string>& list, const std::string& item) {
  int i = 0;
  for (const std::string& it : list) {
    if (!item.compare(it))
      return i;
    i++;
  }
  return -1;
}

inline int indexOf(std::set<std::string>& set, const std::string& item) {
  int i = 0;
  for (const std::string& it : set) {
    if (!item.compare(it))
      return i;
    i++;
  }
  return -1;
}

inline std::wstring tobitstr(DWORD val) {
  std::wostringstream ss;
  ss << std::bitset<32>(val);
  return ss.str();
}

inline std::string tobitstrA(DWORD val) {
  std::ostringstream ss;
  ss << std::bitset<32>(val);
  return ss.str();
}

inline std::string join(std::set<std::string>& set) {
  std::string s;
  int i = 0;
  for (const std::string& it : set) {
    if (i > 0)
      s += ",";
    s += it;
    i++;
  }
  return s;
}

inline std::string join(std::list<std::string>& list) {
  std::string s;
  int i = 0;
  for (const std::string& it : list) {
    if (i > 0)
      s += ",";
    s += it;
    i++;
  }
  return s;
}

inline std::string join(std::vector<std::string>& list) {
  std::string s;
  int i = 0;
  for (const std::string& it : list) {
    if (i > 0)
      s += ",";
    s += it;
    i++;
  }
  return s;
}

inline bool endswith(const std::wstring& s, const std::wstring& ending) {
  if (s.length() >= ending.length()) {
    return (0 ==
            s.compare(s.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}

inline bool startswith(const std::wstring& s, const std::wstring& starting) {
  if (s.length() >= starting.length()) {
    return (0 == s.compare(0, starting.length(), starting));
  } else {
    return false;
  }
}

inline DWORD bit_set_options(std::set<std::string>& set,
                             std::map<std::string, bool>& values) {
  DWORD options = 0;
  int i = 0;
  for (const std::string& it : set) {
    if (values[it])
      options = bit_set(options, i);
    i++;
  }
  return options;
}

static std::string processNameA;
static std::wstring processName;
static bool isLogger = true;

inline LONGLONG timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

inline std::string getDateTimeA() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  auto timer = system_clock::to_time_t(now);
  std::tm tm = *std::localtime(&timer);
  std::ostringstream oss;
  // oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S")
  oss << std::put_time(&tm, "%H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}

inline std::wstring getDateTime() {
  return s2ws(getDateTimeA());
}

inline std::wstring getProcessName() {
  if (processName.empty()) {
    TCHAR name[MAX_PATH];
    DWORD result = GetModuleBaseName(GetCurrentProcess(), NULL, name, MAX_PATH);
    if (result) {
      // logger(L"[" + std::wstring(processName) + L"]", L"Got Process Name");
      processName = std::wstring(name);
      processNameA = ws2s(processName);
      if (!std::wstring(processName).compare(L"Dbgview.exe"))
        isLogger = false;
    } else {
      DWORD error = GetLastError();
      OutputDebugString((L"[" + getDateTime() +
                         L"] Unable to get Process Name [Error: " +
                         std::to_wstring(error) + L"]")
                            .c_str());
    }
  }
  return processName;
}

inline std::string getProcessNameA() {
  getProcessName();
  return processNameA;
}

inline void logger(const std::wstring& log, const std::wstring& content) {
  std::wstring processName = getProcessName();
  if (isLogger) {
    OutputDebugString((L"[" + getDateTime() + L"][" + processName + L"]" + log +
                       L" " + content)
                          .c_str());
  }
}

inline void logger(const std::string& log, const std::string& content) {
  std::string processName = getProcessNameA();
  if (isLogger) {
    OutputDebugStringA(
        ("[" + getDateTimeA() + "][" + processName + "]" + log + " " + content)
            .c_str());
  }
}

inline LPCWSTR getWeaselRegName(HKEY key) {
  LPCWSTR WEASEL_REG_NAME_;
  if (key == HKEY_CURRENT_USER) {
    WEASEL_REG_NAME_ = L"Software\\Rime\\Weasel";
  } else {
    if (is_wow64())
      WEASEL_REG_NAME_ = L"Software\\WOW6432Node\\Rime\\Weasel";
    else
      WEASEL_REG_NAME_ = L"Software\\Rime\\Weasel";
  }
  return WEASEL_REG_NAME_;
}

inline LPCSTR getWeaselRegNameA(HKEY key) {
  LPCSTR WEASEL_REG_NAME_;
  if (key == HKEY_CURRENT_USER) {
    WEASEL_REG_NAME_ = "Software\\Rime\\Weasel";
  } else {
    if (is_wow64())
      WEASEL_REG_NAME_ = "Software\\WOW6432Node\\Rime\\Weasel";
    else
      WEASEL_REG_NAME_ = "Software\\Rime\\Weasel";
  }
  return WEASEL_REG_NAME_;
}

inline std::wstring getHKEYName(HKEY key) {
  if (key == HKEY_CLASSES_ROOT) {
    return L"HKEY_CLASSES_ROOT";
  } else if (key == HKEY_CURRENT_USER) {
    return L"HKEY_CURRENT_USER";
  } else if (key == HKEY_LOCAL_MACHINE) {
    return L"HKEY_LOCAL_MACHINE";
  } else if (key == HKEY_USERS) {
    return L"HKEY_USERS";
  } else if (key == HKEY_PERFORMANCE_DATA) {
    return L"HKEY_PERFORMANCE_DATA";
  } else if (key == HKEY_PERFORMANCE_TEXT) {
    return L"HKEY_PERFORMANCE_TEXT";
  } else if (key == HKEY_PERFORMANCE_NLSTEXT) {
    return L"HKEY_PERFORMANCE_NLSTEXT";
  } else if (key == HKEY_CURRENT_CONFIG) {
    return L"HKEY_CURRENT_CONFIG";
  } else if (key == HKEY_DYN_DATA) {
    return L"HKEY_DYN_DATA";
  } else if (key == HKEY_CURRENT_USER_LOCAL_SETTINGS) {
    return L"HKEY_CURRENT_USER_LOCAL_SETTINGS";
  } else {
    return L"HKEY_UNKNOWN";
  }
}

inline std::string getHKEYNameA(HKEY key) {
  if (key == HKEY_CLASSES_ROOT) {
    return "HKEY_CLASSES_ROOT";
  } else if (key == HKEY_CURRENT_USER) {
    return "HKEY_CURRENT_USER";
  } else if (key == HKEY_LOCAL_MACHINE) {
    return "HKEY_LOCAL_MACHINE";
  } else if (key == HKEY_USERS) {
    return "HKEY_USERS";
  } else if (key == HKEY_PERFORMANCE_DATA) {
    return "HKEY_PERFORMANCE_DATA";
  } else if (key == HKEY_PERFORMANCE_TEXT) {
    return "HKEY_PERFORMANCE_TEXT";
  } else if (key == HKEY_PERFORMANCE_NLSTEXT) {
    return "HKEY_PERFORMANCE_NLSTEXT";
  } else if (key == HKEY_CURRENT_CONFIG) {
    return "HKEY_CURRENT_CONFIG";
  } else if (key == HKEY_DYN_DATA) {
    return "HKEY_DYN_DATA";
  } else if (key == HKEY_CURRENT_USER_LOCAL_SETTINGS) {
    return "HKEY_CURRENT_USER_LOCAL_SETTINGS";
  } else {
    return "HKEY_UNKNOWN";
  }
}

inline std::string getThisAddress(const void* address) {
  std::stringstream ss;
  ss << address;
  return ss.str();
}
