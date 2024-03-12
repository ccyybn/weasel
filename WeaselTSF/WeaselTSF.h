#pragma once

#include <WeaselCommon.h>
#include <RimeWithWeasel.h>
#include "Globals.h"
#include "WeaselIPC.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <psapi.h>

class CCandidateList;
class CLangBarItemButton;
class CCompartmentEventSink;

struct ProcessInfo {
  DWORD processId;
  HANDLE handle;
};

class WeaselTSF : public ITfTextInputProcessorEx,
                  public ITfThreadMgrEventSink,
                  public ITfTextEditSink,
                  public ITfTextLayoutSink,
                  public ITfKeyEventSink,
                  public ITfCompositionSink,
                  public ITfThreadFocusSink,
                  public ITfActiveLanguageProfileNotifySink,
                  public ITfEditSession,
                  public ITfDisplayAttributeProvider {
 public:
  WeaselTSF();
  ~WeaselTSF();
  /* IUnknown */
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  /* ITfTextInputProcessor */
  STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId);
  STDMETHODIMP Deactivate();

  /* ITfTextInputProcessorEx */
  STDMETHODIMP ActivateEx(ITfThreadMgr* pThreadMgr,
                          TfClientId tfClientId,
                          DWORD dwFlags);

  /* ITfThreadMgrEventSink */
  STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDocMgr);
  STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr);
  STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDocMgrFocus,
                          ITfDocumentMgr* pDocMgrPrevFocus);
  STDMETHODIMP OnPushContext(ITfContext* pContext);
  STDMETHODIMP OnPopContext(ITfContext* pContext);

  /* ITfTextEditSink */
  STDMETHODIMP OnEndEdit(ITfContext* pic,
                         TfEditCookie ecReadOnly,
                         ITfEditRecord* pEditRecord);

  /* ITfTextLayoutSink */
  STDMETHODIMP OnLayoutChange(ITfContext* pContext,
                              TfLayoutCode lcode,
                              ITfContextView* pContextView);

  /* ITfKeyEventSink */
  STDMETHODIMP OnSetFocus(BOOL fForeground);
  STDMETHODIMP OnTestKeyDown(ITfContext* pContext,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL* pfEaten);
  STDMETHODIMP OnKeyDown(ITfContext* pContext,
                         WPARAM wParam,
                         LPARAM lParam,
                         BOOL* pfEaten);
  STDMETHODIMP OnTestKeyUp(ITfContext* pContext,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL* pfEaten);
  STDMETHODIMP OnKeyUp(ITfContext* pContext,
                       WPARAM wParam,
                       LPARAM lParam,
                       BOOL* pfEaten);
  STDMETHODIMP OnPreservedKey(ITfContext* pContext,
                              REFGUID rguid,
                              BOOL* pfEaten);

  // ITfThreadFocusSink
  STDMETHODIMP OnSetThreadFocus();
  STDMETHODIMP OnKillThreadFocus();

  /* ITfCompositionSink */
  STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite,
                                       ITfComposition* pComposition);

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

  /* ITfActiveLanguageProfileNotifySink */
  STDMETHODIMP OnActivated(REFCLSID clsid,
                           REFGUID guidProfile,
                           BOOL isActivated);

  // ITfDisplayAttributeProvider
  STDMETHODIMP EnumDisplayAttributeInfo(
      __RPC__deref_out_opt IEnumTfDisplayAttributeInfo** ppEnum);
  STDMETHODIMP GetDisplayAttributeInfo(
      __RPC__in REFGUID guidInfo,
      __RPC__deref_out_opt ITfDisplayAttributeInfo** ppInfo);

  ///* ITfCompartmentEventSink */
  // STDMETHODIMP OnChange(_In_ REFGUID guid);

  /* Compartments */
  BOOL _IsKeyboardDisabled();
  BOOL _IsKeyboardOpen();
  HRESULT _SetKeyboardOpen(BOOL fOpen);
  HRESULT _GetCompartmentDWORD(DWORD& value, const GUID guid);
  HRESULT _SetCompartmentDWORD(const DWORD& value, const GUID guid);

  /* Composition */
  void _StartComposition(com_ptr<ITfContext> pContext,
                         BOOL fCUASWorkaroundEnabled);
  void _EndComposition(com_ptr<ITfContext> pContext, BOOL clear);
  BOOL _ShowInlinePreedit(com_ptr<ITfContext> pContext,
                          const std::shared_ptr<weasel::Context> context);
  void _UpdateComposition(com_ptr<ITfContext> pContext);
  BOOL _IsComposing();
  void _SetComposition(com_ptr<ITfComposition> pComposition);
  void _SetCompositionPosition(const RECT& rc);
  BOOL _UpdateCompositionWindow(com_ptr<ITfContext> pContext);
  void _FinalizeComposition();
  void _AbortComposition(bool clear = true);

  /* Language bar */
  HWND _GetFocusedContextWindow();
  void _HandleLangBarMenuSelect(UINT wID);

  /* IPC */
  void _EnsureServerConnected();
  void _ResetStatusAfterReConnected(const std::wstring& callFrom);

  /* UI */
  void _UpdateUI(const weasel::Context& ctx, const weasel::Status& status);
  void _StartUI();
  void _EndUI();
  void _ShowUI();
  void _HideUI();
  com_ptr<ITfContext> _GetUIContextDocument();

  /* Display Attribute */
  void _ClearCompositionDisplayAttributes(TfEditCookie ec,
                                          _In_ ITfContext* pContext);
  BOOL _SetCompositionDisplayAttributes(TfEditCookie ec,
                                        _In_ ITfContext* pContext,
                                        ITfRange* pRangeComposition);
  BOOL _InitDisplayAttributeGuidAtom();

  com_ptr<ITfThreadMgr> _GetThreadMgr() { return _pThreadMgr; }
  void HandleUICallback(size_t* const sel,
                        size_t* const hov,
                        bool* const next,
                        bool* const scroll_next);

  BOOL _IsGuard();
  void _SwitchGuard(BOOL fOpen);

 private:
  /* ui callback functions private */
  void _SelectCandidateOnCurrentPage(const size_t index);
  void _HandleMouseHoverEvent(const size_t index);
  void _HandleMousePageEvent(bool* const nextPage, bool* const scrollNextPage);
  /* TSF Related */
  BOOL _InitThreadMgrEventSink();
  void _UninitThreadMgrEventSink();

  BOOL _InitTextEditSink(com_ptr<ITfDocumentMgr> pDocMgr);

  BOOL _InitKeyEventSink();
  void _UninitKeyEventSink();
  void _ProcessKeyEvent(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
  ProcessInfo _CheckServer(const std::wstring& log, BOOL isEnsure);
  void _CheckServerOnEnsure();
  void _StartSeverAndReconnect(const std::wstring& log);
  void _StartServerGuardian();
  void _InitializeCompartment();

  BOOL _InitPreservedKey();
  void _UninitPreservedKey();

  BOOL _InitLanguageBar();
  void _UninitLanguageBar();
  void _UpdateLanguageBar(const std::wstring& callFrom, weasel::Status stat);
  void _ShowLanguageBar(BOOL show);
  void _EnableLanguageBar(BOOL enable);

  BOOL _InsertText(com_ptr<ITfContext> pContext, const std::wstring& ext);

  void _DeleteCandidateList();

  BOOL _InitCompartment();
  void _UninitCompartment();
  HRESULT _HandleCompartment(REFGUID guidCompartment);

  bool isImmersive() const {
    return (_activateFlags & TF_TMF_IMMERSIVEMODE) != 0;
  }

  weasel::Status _backupStatus;
  bool _isBackupStatusInitialized = false;
  std::set<std::string> _allOptions;

  com_ptr<ITfCompartment> _pIsGuardCompartment;
  com_ptr<ITfThreadMgr> _pThreadMgr;
  TfClientId _tfClientId;
  DWORD _dwThreadMgrEventSinkCookie;

  com_ptr<ITfContext> _pTextEditSinkContext;
  DWORD _dwTextEditSinkCookie, _dwTextLayoutSinkCookie;
  BYTE _lpbKeyState[256];
  BOOL _fTestKeyDownPending, _fTestKeyUpPending;

  com_ptr<ITfContext> _pEditSessionContext;
  std::wstring _editSessionText;

  com_ptr<CCompartmentEventSink> _pKeyboardCompartmentSink;
  com_ptr<CCompartmentEventSink> _pConvertionCompartmentSink;

  com_ptr<ITfComposition> _pComposition;

  com_ptr<CLangBarItemButton> _pLangBarButton;

  com_ptr<CCandidateList> _cand;

  LONG _cRef;  // COM ref count

  /* CUAS Candidate Window Position Workaround */
  BOOL _fCUASWorkaroundTested, _fCUASWorkaroundEnabled;

  /* Weasel Related */
  weasel::Client m_client;
  DWORD _activateFlags;

  /* IME status */
  weasel::Status _status;

  // guidatom for the display attibute.
  TfGuidAtom _gaDisplayAttributeInput;
};

static LONGLONG lastCheckServerTime = 0;
static BOOL isInEnsure = FALSE;
static LONGLONG checkServerInterval = 1000;

static std::mutex initializeMutex;

static BOOL isGuardianStarted = FALSE;
static std::mutex isGuardianStartedMutex;

static BOOL isFirstEnsure = TRUE;
static std::mutex isFirstEnsureMutex;

static std::wstring weaselServerProcessName = L"WeaselServer.exe";
static LPCWSTR weaselServerGuardianMutex = L"Global\\WeaselServerGuardianMutex";
static LPCWSTR weaselServerEnsureMutex = L"Global\\WeaselServerEnsureMutex";
static LPCWSTR weaselServerStartingMutex = L"Global\\WeaselServerStartingMutex";

static std::wstring weaselTSFID = std::to_wstring(timeSinceEpochMillisec());

static DWORD updateOptionValueByNewList(
    std::wstring log,
    DWORD backup_options,
    std::set<std::string> backup_option_list,
    std::set<std::string> new_option_list) {
  DWORD options = 0;

  std::set<std::string>::iterator it;
  int i = 0;
  for (it = new_option_list.begin(); it != new_option_list.end(); ++it) {
    std::string option_name = *it;
    int index = indexOf(backup_option_list, option_name);
    if (index >= 0) {
      if (bit_check(backup_options, index))
        options = bit_set(options, i);
    }
    i++;
  }
  logger(log, L"Option Values Reset[" + tobitstr(backup_options) + L"] -> [" +
                  tobitstr(options) + L"]");
  return options;
}

static DWORD FindProcessId(const std::wstring& processName) {
  PROCESSENTRY32 processInfo;
  processInfo.dwSize = sizeof(processInfo);

  HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (processesSnapshot == INVALID_HANDLE_VALUE)
    return 0;

  Process32First(processesSnapshot, &processInfo);

  if (!processName.compare(processInfo.szExeFile)) {
    CloseHandle(processesSnapshot);
    return processInfo.th32ProcessID;
  }

  while (Process32Next(processesSnapshot, &processInfo)) {
    if (!processName.compare(processInfo.szExeFile)) {
      CloseHandle(processesSnapshot);
      return processInfo.th32ProcessID;
    }
  }

  CloseHandle(processesSnapshot);
  return 0;
}

static ProcessInfo startProcess(std::wstring log,
                                std::wstring dir,
                                std::wstring file) {
  ProcessInfo info{};
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);

  std::wstring path = dir + L"\\" + file;
  if (!CreateProcess(NULL, &path[0], NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
                     NULL, dir.c_str(), &si, &pi)) {
    DWORD error = GetLastError();
    logger(log, L"[" + path + L"] Unable to start the process [Error: " +
                    std::to_wstring(error) + L"]");
  } else {
    logger(log, L"[" + path + L"] Process started [PID: " +
                    std::to_wstring(pi.dwProcessId) + L"]");
    info.processId = pi.dwProcessId;
    info.handle = pi.hProcess;
  }
  return info;
}
static ProcessInfo startService(std::wstring log) {
  std::wstring dir;
  LONG lRes = RegGetStringValue(HKEY_LOCAL_MACHINE,
                                getWeaselRegName(HKEY_LOCAL_MACHINE),
                                L"WeaselRoot", dir);
  ProcessInfo info{};

  if (lRes == ERROR_SUCCESS) {
    return startProcess(log, dir, L"WeaselServer.exe");
  } else {
    ProcessInfo info{};
    return info;
  }
}

static HANDLE createMutex(std::wstring log, LPCWSTR lpName) {
  BOOL isOwner = TRUE;
  HANDLE hMutex = CreateMutex(NULL, FALSE, lpName);
  if (!hMutex) {
    DWORD error = GetLastError();
    logger(log, L"Failed to create mutex [" + std::wstring(lpName) +
                    L"][Error: " + std::to_wstring(error) + L"]");
  }
  return hMutex;
}

static BOOL handleWaitForSingleObject(std::wstring log,
                                      std::wstring mutexName,
                                      DWORD ret) {
  BOOL isOwner = FALSE;
  if (ret == WAIT_ABANDONED) {
    logger(log, L"Obtained Mutex [" + mutexName + L"] WAIT_ABANDONED");
    isOwner = TRUE;
  } else if (ret == WAIT_OBJECT_0) {
    logger(log, L"Obtained Mutex [" + mutexName + L"] WAIT_OBJECT_0");
    isOwner = TRUE;
  } else if (ret == WAIT_TIMEOUT) {
    logger(log, L"Mutex [" + mutexName + L"] WAIT_TIMEOUT");
  } else if (ret == WAIT_FAILED) {
    DWORD error = GetLastError();
    logger(log, L"Mutex [" + mutexName + L"] WAIT_FAILED [Error: " +
                    std::to_wstring(error) + L"]");
  } else {
    logger(log, L"Mutex [" + mutexName + L"] WAIT_UNKNOWN");
  }
  return isOwner;
}

static std::wstring handleToString(HANDLE handle) {
  std::wostringstream ss;
  ss << std::hex << handle;
  std::wstring strHandle = ss.str();
  return strHandle;
}
