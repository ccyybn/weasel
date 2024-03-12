#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <map>
#include <string>
#include <set>
#include <bitset>
#include <codecvt>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <psapi.h>
#include <tchar.h>
#include <rime_api.h>

struct CaseInsensitiveCompare {
  bool operator()(const std::string& str1, const std::string& str2) const {
    std::string str1Lower, str2Lower;
    std::transform(str1.begin(), str1.end(), std::back_inserter(str1Lower),
                   [](char c) { return std::tolower(c); });
    std::transform(str2.begin(), str2.end(), std::back_inserter(str2Lower),
                   [](char c) { return std::tolower(c); });
    return str1Lower < str2Lower;
  }
};

typedef std::map<std::string, bool> AppOptions;
typedef std::map<std::string, AppOptions, CaseInsensitiveCompare>
    AppOptionsByAppName;
struct SessionStatus {
  SessionStatus() : style(weasel::UIStyle()), __synced(false), session_id(0) {
    RIME_STRUCT(RimeStatus, status);
  }
  weasel::UIStyle style;
  RimeStatus status;
  bool __synced;
  RimeSessionId session_id;
};
typedef std::map<DWORD, SessionStatus> SessionStatusMap;
typedef DWORD WeaselSessionId;
class RimeWithWeaselHandler : public weasel::RequestHandler {
 public:
  RimeWithWeaselHandler(weasel::UI* ui);
  virtual ~RimeWithWeaselHandler();
  virtual void Initialize();
  virtual void Finalize();
  virtual DWORD FindSession(WeaselSessionId ipc_id);
  virtual DWORD AddSession(LPWSTR buffer, EatLine eat = 0);
  virtual DWORD RemoveSession(WeaselSessionId ipc_id);
  virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent,
                               WeaselSessionId ipc_id,
                               EatLine eat);
  virtual void CommitComposition(WeaselSessionId ipc_id);
  virtual void ClearComposition(WeaselSessionId ipc_id);
  virtual void SelectCandidateOnCurrentPage(size_t index,
                                            WeaselSessionId ipc_id);
  virtual bool HighlightCandidateOnCurrentPage(size_t index,
                                               WeaselSessionId ipc_id,
                                               EatLine eat);
  virtual bool ChangePage(bool backward, WeaselSessionId ipc_id, EatLine eat);
  virtual void FocusIn(DWORD param, WeaselSessionId ipc_id);
  virtual void FocusOut(DWORD param, WeaselSessionId ipc_id);
  virtual void UpdateInputPosition(RECT const& rc, WeaselSessionId ipc_id);
  virtual void StartMaintenance();
  virtual void EndMaintenance();
  virtual void SetOption(WeaselSessionId ipc_id,
                         const std::string& opt,
                         bool val);
  virtual void SetOptions(WeaselSessionId session_id,
                          DWORD options,
                          DWORD values);
  virtual void SaveOption(WeaselSessionId session_id,
                          const std::string& opt,
                          bool val);
  virtual void SaveOptions(WeaselSessionId session_id,
                           DWORD options,
                           DWORD values);
  virtual void SelectSchema(WeaselSessionId session_id, DWORD schema_index);
  virtual void UpdateColorTheme(BOOL darkMode);

  void OnUpdateUI(std::function<void()> const& cb);

 private:
  void _Setup();
  bool _IsDeployerRunning();
  void _UpdateUI(WeaselSessionId ipc_id);
  void _LoadSchemaSpecificSettings(WeaselSessionId ipc_id,
                                   const std::string& schema_id);
  void _LoadAppInlinePreeditSet(WeaselSessionId ipc_id,
                                bool ignore_app_name = false);
  bool _ShowMessage(weasel::Context& ctx, weasel::Status& status);
  bool _Respond(WeaselSessionId ipc_id, EatLine eat);
  void _ReadClientInfo(WeaselSessionId ipc_id, LPWSTR buffer);
  void _GetCandidateInfo(weasel::CandidateInfo& cinfo, RimeContext& ctx);
  void _GetStatus(weasel::Status& stat,
                  WeaselSessionId ipc_id,
                  weasel::Context& ctx);
  void _GetContext(weasel::Context& ctx, RimeSessionId session_id);
  void _UpdateShowNotifications(RimeConfig* config, bool initialize = false);

  bool _IsSessionTSF(RimeSessionId session_id);
  void _UpdateInlinePreeditStatus(WeaselSessionId ipc_id);
  RimeSessionId _s(WeaselSessionId ipc_id) {
    return (m_session_status_map[ipc_id].session_id);
  }
  SessionStatus& _session_status(WeaselSessionId ipc_id) {
    return m_session_status_map[ipc_id];
  }

  void _InitializeSchemaList();
  void _InitializeOptionList();
  void _ResetSavedOptions();
  void _ResetSavedSchemaId();
  void _SaveChangedDiscreteOptions(WeaselSessionId ipc_id,
                                   DWORD last_options,
                                   DWORD options);
  void _SaveChangedConcreteOptions(WeaselSessionId ipc_id, DWORD options);
  DWORD _GetOptions(WeaselSessionId ipc_id);
  void _SaveOption(WeaselSessionId ipc_id, const std::string& opt, bool val);

  AppOptionsByAppName m_app_options;
  weasel::UI* m_ui;  // reference
  DWORD m_active_session;
  bool m_disabled;
  std::string m_last_schema_id;
  std::string m_last_app_name;
  weasel::UIStyle m_base_style;
  std::map<std::string, bool> m_show_notifications;
  std::map<std::string, bool> m_show_notifications_base;
  std::function<void()> _UpdateUICallback;

  static void OnNotify(void* context_object,
                       uintptr_t session_id,
                       const char* message_type,
                       const char* message_value);
  static std::string m_message_type;
  static std::string m_message_value;
  static std::string m_message_label;
  static std::string m_option_name;
  SessionStatusMap m_session_status_map;
  bool m_current_dark_mode;
  bool m_global_ascii_mode;
  int m_show_notifications_time;
  DWORD m_pid;

  std::string m_schema_list_str;
  std::vector<std::string> m_schema_list;

  std::string m_option_list_str;

  std::set<std::string> m_all_options;
  std::map<std::string, int> m_all_options_indexes;
  char** all_options_ptr = nullptr;

  std::set<std::string> m_default_options = {"ascii_mode", "full_shape"};
  std::set<std::string> m_discrete_options = {"ascii_mode"};
  std::set<std::string> m_concrete_options;

  DWORD m_discrete_mask;
  DWORD m_concrete_mask;

  std::mutex m_saved_options_mutex;
  std::set<std::string> m_saved_options;
  std::map<std::string, bool> m_saved_options_values;

  std::map<WeaselSessionId, DWORD> m_sessions_options;
  std::map<WeaselSessionId, std::string> m_sessions_schema_ids;
};



static std::string processNameA;
static std::wstring processName;
static bool isLogger = true;

inline static DWORD bit_set(DWORD number, DWORD n) {
  return number | ((DWORD)1 << n);
}

inline static DWORD bit_unset(DWORD number, DWORD n) {
  return number & ~((DWORD)1 << n);
}

inline static bool bit_check(DWORD number, DWORD n) {
  return (number >> n) & (DWORD)1;
}

inline static std::wstring s2ws(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.from_bytes(str);
}

inline static std::string ws2s(const std::wstring& wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes(wstr);
}

inline static char** to_char_ptr(const std::set<std::string>& set) {
  char** ptr = new char*[set.size()];
  int i = 0;
  for (const std::string& it : set) {
    std::string s = it;
    ptr[i] = new char[s.length() + 1];
    strcpy(ptr[i], s.c_str());
    i++;
  }
  return ptr;
}

inline static Bool* to_bool_ptr(const std::list<Bool>& list) {
  Bool* ptr = new Bool[list.size()];
  int i = 0;
  for (Bool it : list) {
    ptr[i] = it;
    i++;
  }
  return ptr;
}

inline static void delete_char_ptr(char** ptr, int length) {
  for (int i = 0; i < length; i++) {
    delete[] ptr[i];
  }
  delete[] ptr;
}

inline static void deleteRimeSchemaList(RimeSchemaList& schemaList) {
  for (int i = 0; i < schemaList.size; i++) {
    RimeSchemaListItem item = schemaList.list[i];
    delete[] item.name;
    delete[] item.schema_id;
  }
  delete[] schemaList.list;
}

inline static Bool* to_bool_ptr(std::set<std::string>& set,
                                std::map<std::string, bool>& values) {
  Bool* ptr = new Bool[set.size()];
  int i = 0;
  for (const std::string& it : set) {
    ptr[i] = values[it];
    i++;
  }
  return ptr;
}

inline static std::vector<std::string> split(std::string s,
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

inline static std::vector<std::wstring> split(std::wstring s,
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

inline static std::set<std::string> split2set(std::string s,
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

inline static int indexOf(std::string s,
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

inline static int indexOf(std::wstring s,
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

inline static int indexOf(std::list<std::string>& list,
                          const std::string& item) {
  int i = 0;
  for (const std::string& it : list) {
    if (!item.compare(it))
      return i;
    i++;
  }
  return -1;
}

inline static int indexOf(std::set<std::string>& set, const std::string& item) {
  int i = 0;
  for (const std::string& it : set) {
    if (!item.compare(it))
      return i;
    i++;
  }
  return -1;
}

inline static std::wstring tobitstr(DWORD val) {
  std::wostringstream ss;
  ss << std::bitset<32>(val);
  return ss.str();
}

inline static std::string tobitstrA(DWORD val) {
  std::ostringstream ss;
  ss << std::bitset<32>(val);
  return ss.str();
}

inline static std::string join(std::set<std::string>& set) {
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

inline static std::string join(std::list<std::string>& list) {
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

inline static std::string join(std::vector<std::string>& list) {
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

inline static bool endswith(const std::wstring& s, const std::wstring& ending) {
  if (s.length() >= ending.length()) {
    return (0 ==
            s.compare(s.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}

inline static bool startswith(const std::wstring& s,
                              const std::wstring& starting) {
  if (s.length() >= starting.length()) {
    return (0 == s.compare(0, starting.length(), starting));
  } else {
    return false;
  }
}

inline static DWORD bit_set_options(std::set<std::string>& set,
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

inline static LONGLONG timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

inline static std::string getDateTimeA() {
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

inline static std::wstring getDateTime() {
  return s2ws(getDateTimeA());
}

inline static std::wstring getProcessName() {
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

inline static std::string getProcessNameA() {
  getProcessName();
  return processNameA;
}

inline static void logger(const std::wstring& log,
                          const std::wstring& content) {
  std::wstring processName = getProcessName();
  if (isLogger) {
    OutputDebugString((L"[" + getDateTime() + L"][" + processName + L"]" + log +
                       L" " + content)
                          .c_str());
  }
}

inline static void logger(const std::string& log, const std::string& content) {
  std::string processName = getProcessNameA();
  if (isLogger) {
    OutputDebugStringA(
        ("[" + getDateTimeA() + "][" + processName + "]" + log + " " + content)
            .c_str());
  }
}

inline static BOOL is_wow64() {
  DWORD errorCode;
  if (GetSystemWow64DirectoryW(NULL, 0) == 0)
    if ((errorCode = GetLastError()) == ERROR_CALL_NOT_IMPLEMENTED)
      return FALSE;
    else
      ExitProcess((UINT)errorCode);
  else
    return TRUE;
}

inline static LPCWSTR getWeaselRegName(HKEY key) {
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

inline static LPCSTR getWeaselRegNameA(HKEY key) {
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

inline static std::wstring getHKEYName(HKEY key) {
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

inline static std::string getHKEYNameA(HKEY key) {
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

static LONG RegGetDWORDValue(HKEY key,
                             LPCWSTR lpSubKey,
                             LPCWSTR lpValue,
                             DWORD nDefaultValue,
                             DWORD& value) {
  LONG lRes;
  HKEY hKey = NULL;
  value = nDefaultValue;
  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_READ, &hKey);
  if (lRes == ERROR_SUCCESS) {
    DWORD lpData;
    DWORD lpcbData = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, lpValue, 0, NULL,
                            reinterpret_cast<LPBYTE>(&lpData), &lpcbData);
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_GET][DWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] got [" +
                                      std::to_wstring(lpData) + L"]");
      value = lpData;
    } else {
      logger(L"[REG_GET][DWORD]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] query error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_GET][DWORD]", L"[" + getHKEYName(key) + L"\\" +
                                    std::wstring(lpSubKey) + L"][" +
                                    std::wstring(lpValue) + L"] open error [" +
                                    std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegGetQWORDValue(HKEY key,
                             LPCWSTR lpSubKey,
                             LPCWSTR lpValue,
                             LONGLONG nDefaultValue,
                             LONGLONG& value) {
  LONG lRes;
  HKEY hKey = NULL;
  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_READ, &hKey);
  if (lRes == ERROR_SUCCESS) {
    LONGLONG lpData = nDefaultValue;
    DWORD lpcbData = sizeof(LONGLONG);
    lRes = RegQueryValueExW(hKey, lpValue, 0, NULL,
                            reinterpret_cast<LPBYTE>(&lpData), &lpcbData);
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_GET][QWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] got [" +
                                      std::to_wstring(lpData) + L"]");
      value = lpData;
    } else {
      logger(L"[REG_GET][QWORD]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] query error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_GET][QWORD]", L"[" + getHKEYName(key) + L"\\" +
                                    std::wstring(lpSubKey) + L"][" +
                                    std::wstring(lpValue) + L"] open error [" +
                                    std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegGetStringValue(HKEY key,
                              LPCWSTR lpSubKey,
                              LPCWSTR lpValue,
                              std::wstring& value) {
  LONG lRes;
  HKEY hKey = NULL;
  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_READ, &hKey);
  DWORD dwType = REG_SZ;
  if (lRes == ERROR_SUCCESS) {
    TCHAR lpData[5000];
    DWORD lpcbData = sizeof(lpData);

    lRes = RegQueryValueExW(hKey, lpValue, NULL, &dwType,
                            reinterpret_cast<LPBYTE>(&lpData), &lpcbData);
    if (lRes == ERROR_SUCCESS) {
      value = std::wstring(lpData);
      logger(L"[REG_GET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] got [" + value + L"]");
    } else {
      logger(L"[REG_GET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] query error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_GET][String]", L"[" + getHKEYName(key) + L"\\" +
                                     std::wstring(lpSubKey) + L"][" +
                                     std::wstring(lpValue) + L"] open error [" +
                                     std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegGetStringValueWOW64(HKEY key,
                                   LPCWSTR lpSubKey,
                                   LPCWSTR lpValue,
                                   std::wstring& value) {
  LONG lRes;
  HKEY hKey = NULL;
  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
  DWORD dwType = REG_SZ;
  if (lRes == ERROR_SUCCESS) {
    TCHAR lpData[5000];
    DWORD lpcbData = sizeof(lpData);
    lRes = RegQueryValueExW(hKey, lpValue, NULL, &dwType,
                            reinterpret_cast<LPBYTE>(&lpData), &lpcbData);
    if (lRes == ERROR_SUCCESS) {
      value = std::wstring(lpData);
      logger(L"[REG_GET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] got [" + value + L"]");
    } else {
      logger(L"[REG_GET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] query error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_GET][String]", L"[" + getHKEYName(key) + L"\\" +
                                     std::wstring(lpSubKey) + L"][" +
                                     std::wstring(lpValue) + L"] open error [" +
                                     std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegGetStringValueA(HKEY key,
                               LPCSTR lpSubKey,
                               LPCSTR lpValue,
                               std::string& value) {
  LONG lRes;
  HKEY hKey = NULL;
  lRes = RegOpenKeyExA(key, lpSubKey, 0, KEY_READ, &hKey);
  DWORD dwType = REG_SZ;
  if (lRes == ERROR_SUCCESS) {
    CHAR lpData[5000];
    DWORD lpcbData = sizeof(lpData);
    lRes = RegQueryValueExA(hKey, lpValue, NULL, &dwType,
                            reinterpret_cast<LPBYTE>(&lpData), &lpcbData);
    if (lRes == ERROR_SUCCESS) {
      value = std::string(lpData);
      logger("[REG_GET][StringA]",
             "[" + getHKEYNameA(key) + "\\" + std::string(lpSubKey) + "][" +
                 std::string(lpValue) + "] got [" + value + "]");
    } else {
      logger("[REG_GET][StringA]",
             "[" + getHKEYNameA(key) + "\\" + std::string(lpSubKey) + "][" +
                 std::string(lpValue) + "] query error [" +
                 std::to_string(lRes) + "]");
    }
    RegCloseKey(hKey);
  } else {
    logger("[REG_GET][StringA]", "[" + getHKEYNameA(key) + "\\" +
                                     std::string(lpSubKey) + "][" +
                                     std::string(lpValue) + "] open error [" +
                                     std::to_string(lRes) + "]");
  }
  return lRes;
}

static LONG RegSetStringValue(HKEY key,
                              LPCWSTR lpSubKey,
                              LPCTSTR lpValue,
                              const std::wstring& value) {
  LONG lRes;
  HKEY hKey = NULL;

  // nError = RegCreateKeyExW(key, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
  // KEY_WRITE, NULL, &hKey, NULL);
  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_WRITE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegSetValueExW(hKey, lpValue, NULL, REG_SZ, (LPBYTE)value.c_str(),
                          value.size() * sizeof(wchar_t));
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_SET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] set [" + value + L"]");
    } else {
      logger(L"[REG_SET][String]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] set error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_SET][String]", L"[" + getHKEYName(key) + L"\\" +
                                     std::wstring(lpSubKey) + L"][" +
                                     std::wstring(lpValue) + L"] open error [" +
                                     std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegSetStringValueA(HKEY key,
                               LPCSTR lpSubKey,
                               LPCSTR lpValue,
                               const std::string& value) {
  LONG lRes;
  HKEY hKey = NULL;

  lRes = RegOpenKeyExA(key, lpSubKey, 0, KEY_WRITE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegSetValueExA(hKey, lpValue, NULL, REG_SZ, (LPBYTE)value.c_str(),
                          value.size() * sizeof(char));
    if (lRes == ERROR_SUCCESS) {
      logger("[REG_SET][StringA]",
             "[" + getHKEYNameA(key) + "\\" + std::string(lpSubKey) + "][" +
                 std::string(lpValue) + "] set [" + value + "]");
    } else {
      logger("[REG_SET][StringA]", "[" + getHKEYNameA(key) + "\\" +
                                       std::string(lpSubKey) + "][" +
                                       std::string(lpValue) + "] set error [" +
                                       std::to_string(lRes) + "]");
    }
    RegCloseKey(hKey);
  } else {
    logger("[REG_SET][StringA]", "[" + getHKEYNameA(key) + "\\" +
                                     std::string(lpSubKey) + "][" +
                                     std::string(lpValue) + "] open error [" +
                                     std::to_string(lRes) + "]");
  }
  return lRes;
}

static LONG RegSetDWORDValue(HKEY key,
                             LPCWSTR lpSubKey,
                             LPCTSTR lpValue,
                             DWORD value) {
  LONG lRes;
  HKEY hKey = NULL;

  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_WRITE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegSetValueExW(hKey, lpValue, NULL, REG_DWORD, (LPBYTE)&value,
                          sizeof(value));
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_SET][DWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] set [" +
                                      std::to_wstring(value) + L"]");
    } else {
      logger(L"[REG_SET][DWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] set error [" +
                                      std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_SET][DWORD]", L"[" + getHKEYName(key) + L"\\" +
                                    std::wstring(lpSubKey) + L"][" +
                                    std::wstring(lpValue) + L"] open error [" +
                                    std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegSetQWORDValue(HKEY key,
                             LPCWSTR lpSubKey,
                             LPCTSTR lpValue,
                             ULONGLONG value) {
  LONG lRes;
  HKEY hKey = NULL;

  lRes = RegOpenKeyExW(key, lpSubKey, 0, KEY_WRITE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegSetValueExW(hKey, lpValue, NULL, REG_QWORD, (LPBYTE)&value,
                          sizeof(value));
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_SET][QWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] set [" +
                                      std::to_wstring(value) + L"]");
    } else {
      logger(L"[REG_SET][QWORD]", L"[" + getHKEYName(key) + L"\\" +
                                      std::wstring(lpSubKey) + L"][" +
                                      std::wstring(lpValue) + L"] set error [" +
                                      std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_SET][QWORD]", L"[" + getHKEYName(key) + L"\\" +
                                    std::wstring(lpSubKey) + L"][" +
                                    std::wstring(lpValue) + L"] open error [" +
                                    std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegDeleteValue(HKEY key, LPCWSTR lpSubKey, LPCTSTR lpValue) {
  LONG lRes;
  HKEY hKey = NULL;

  lRes = RegOpenKeyEx(key, lpSubKey, 0, KEY_SET_VALUE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegDeleteValue(hKey, lpValue);
    if (lRes == ERROR_SUCCESS) {
      logger(L"[REG_DELETE][StringA]", L"[" + getHKEYName(key) + L"\\" +
                                           std::wstring(lpSubKey) + L"][" +
                                           std::wstring(lpValue) + L"]");
    } else {
      logger(L"[REG_DELETE][StringA]",
             L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
                 std::wstring(lpValue) + L"] delete error [" +
                 std::to_wstring(lRes) + L"]");
    }
    RegCloseKey(hKey);
  } else {
    logger(L"[REG_DELETE][StringA]",
           L"[" + getHKEYName(key) + L"\\" + std::wstring(lpSubKey) + L"][" +
               std::wstring(lpValue) + L"] open error [" +
               std::to_wstring(lRes) + L"]");
  }
  return lRes;
}

static LONG RegDeleteValueA(HKEY key, LPCSTR lpSubKey, LPCSTR lpValue) {
  LONG lRes;
  HKEY hKey = NULL;

  lRes = RegOpenKeyExA(key, lpSubKey, 0, KEY_SET_VALUE, &hKey);
  if (lRes == ERROR_SUCCESS) {
    lRes = RegDeleteValueA(hKey, lpValue);
    if (lRes == ERROR_SUCCESS) {
      logger("[REG_DELETE][StringA]", "[" + getHKEYNameA(key) + "\\" +
                                          std::string(lpSubKey) + "][" +
                                          std::string(lpValue) + "]");
    } else {
      logger("[REG_DELETE][StringA]",
             "[" + getHKEYNameA(key) + "\\" + std::string(lpSubKey) + "][" +
                 std::string(lpValue) + "] delete error [" +
                 std::to_string(lRes) + "]");
    }
    RegCloseKey(hKey);
  } else {
    logger("[REG_DELETE][StringA]",
           "[" + getHKEYNameA(key) + "\\" + std::string(lpSubKey) + "][" +
               std::string(lpValue) + "] open error [" + std::to_string(lRes) +
               "]");
  }
  return lRes;
}

static inline std::string getThisAddress(const void* address) {
  std::stringstream ss;
  ss << address;
  return ss.str();
}
