#include "stdafx.h"

#include <resource.h>
#include "WeaselTSF.h"
#include "WeaselCommon.h"
#include "CandidateList.h"
#include "LanguageBar.h"
#include "Compartment.h"
#include "ResponseParser.h"

static void error_message(const WCHAR* msg) {
  static DWORD next_tick = 0;
  DWORD now = GetTickCount();
  if (now > next_tick) {
    next_tick = now + 10000;  // (ms)
    MessageBox(NULL, msg, TEXTSERVICE_DESC, MB_ICONERROR | MB_OK);
  }
}

BOOL WeaselTSF::_IsGuard() {
  BOOL fOpen = TRUE;

  VARIANT var;
  if (_pIsGuardCompartment != NULL) {
    HRESULT hr;
    hr = _pIsGuardCompartment->GetValue(&var);

    if (hr == S_OK) {
      if (var.vt == VT_I4) {
        fOpen = (BOOL)var.lVal;
      } else {
        OutputDebugString(L"_IsGuard var.vt is not VT_I4");
      }
    } else if (hr == S_FALSE) {
      // OutputDebugString(L"_IsGuard GetValue S_FALSE");
    } else if (hr == E_UNEXPECTED) {
      OutputDebugString(L"_IsGuard GetValue E_UNEXPECTED");
    } else if (hr == E_INVALIDARG) {
      OutputDebugString(L"_IsGuard GetValue E_INVALIDARG");
    } else {
      OutputDebugString(L"_IsGuard GetValue UNKNOWN ERROR");
    }
  } else {
    OutputDebugString(L"_IsGuard GetValue _pIsGuardCompartment is null");
  }
  return fOpen;
}

void WeaselTSF::_InitializeCompartment() {
  initializeMutex.lock();
  if (_pIsGuardCompartment == NULL) {
    com_ptr<ITfCompartmentMgr> pCompMgr;
    HRESULT ret = _pThreadMgr->GetGlobalCompartment(&pCompMgr);
    pCompMgr->GetCompartment(GUID_COMPARTMENT_IS_GUARD, &_pIsGuardCompartment);
  }
  initializeMutex.unlock();
}

void WeaselTSF::_SwitchGuard(BOOL fOpen) {
  if (_pIsGuardCompartment != NULL) {
    HRESULT hr = E_FAIL;
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = fOpen;
    hr = _pIsGuardCompartment->SetValue(_tfClientId, &var);
  }
}

static ProcessInfo waitForAnotherProcess(const std::wstring& log,
                                         HANDLE hMutex) {
  BOOL isStart = FALSE;
  DWORD ret = WaitForSingleObject(hMutex, INFINITE);
  BOOL isOwner = handleWaitForSingleObject(log, weaselServerStartingMutex, ret);
  HANDLE handle = NULL;
  ProcessInfo info{};
  if (isOwner) {
    if (ret == WAIT_OBJECT_0) {
      info.processId = FindProcessId(weaselServerProcessName);
      logger(log, L"Another process has started Weasel Server [PID: " +
                      std::to_wstring(info.processId) + L"]");
    } else {
      logger(log, L"Another process has crashed, starting Weasel Server");
      info = startService(log);
      Sleep(150);
    }
    ReleaseMutex(hMutex);
    logger(log, L"Released Mutex [" + std::wstring(weaselServerStartingMutex) +
                    L"]");
  } else {
    logger(log, L"Unable to wait another process");
  }
  return info;
}

ProcessInfo WeaselTSF::_CheckServer(const std::wstring& log,
                                    BOOL isEnsure = FALSE) {
  LONGLONG now = timeSinceEpochMillisec();
  lastCheckServerTime = now;
  LONGLONG nextSleepTime = checkServerInterval;
  ProcessInfo info{};
  DWORD processId = FindProcessId(weaselServerProcessName);
  if (_IsGuard()) {
    // logname = logname + L"[Last: " + std::to_wstring(lastCheckServerTime)
    // +L"]";
    logger(log, L"Checking");
    if (!processId) {
      HANDLE hMutex = createMutex(log, weaselServerStartingMutex);
      if (hMutex) {
        DWORD ret = WaitForSingleObject(hMutex, 0);
        BOOL isOwner =
            handleWaitForSingleObject(log, weaselServerStartingMutex, ret);
        if (isOwner) {
          logger(log, L"Starting Weasel Server");
          info = startService(log);
          Sleep(150);
          ReleaseMutex(hMutex);
          logger(log, L"Released Mutex [" +
                          std::wstring(weaselServerStartingMutex) + L"]");
        } else {
          logger(log, L"Another process is starting Weasel Server, waiting");
          info = waitForAnotherProcess(log, hMutex);
        }
        CloseHandle(hMutex);
      }
    } else if (isEnsure) {
      logger(log, L"Weasel Server exists, try to check [" +
                      std::wstring(weaselServerStartingMutex) + L"]");
      HANDLE hMutex = createMutex(log, weaselServerStartingMutex);
      if (hMutex) {
        info = waitForAnotherProcess(log, hMutex);
        CloseHandle(hMutex);
      }
    } else {
      info.processId = processId;
    }
  }
  return info;
}

void WeaselTSF::_StartSeverAndReconnect(const std::wstring& log) {
  HANDLE startingMutex = NULL;
  _CheckServer(log, TRUE);
  if (!m_client.Echo()) {
    logger(log, L"Reconnect client");
    m_client.Disconnect();
    m_client.Connect(NULL);
    m_client.StartSession();
  }
}

void WeaselTSF::_CheckServerOnEnsure() {
  _InitializeCompartment();
  if (!isInEnsure && _IsGuard()) {
    isInEnsure = TRUE;
    std::wstring log = L"[WeaselTSF][Ensure]";

    HANDLE hMutex = createMutex(log, weaselServerEnsureMutex);
    if (hMutex) {
      DWORD ret = WaitForSingleObject(hMutex, 0);
      BOOL isOwner =
          handleWaitForSingleObject(log, weaselServerEnsureMutex, ret);
      if (isOwner) {
        _StartSeverAndReconnect(log);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        logger(log, L"Released Mutex [" +
                        std::wstring(weaselServerEnsureMutex) + L"]");
      } else {
        logger(log, L"Another process is ensuring Weasel Server");
        CloseHandle(hMutex);
      }
    }
    isInEnsure = FALSE;
  }
}

static ProcessInfo openServerHandle(const std::wstring& log,
                                    ProcessInfo& processInfo) {
  if (processInfo.handle) {
    logger(log, L"Started WeaselServer [PID: " +
                    std::to_wstring(processInfo.processId) + L"][" +
                    handleToString(processInfo.handle) + L"]");
  } else if (processInfo.processId) {
    processInfo.handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE,
        FALSE, processInfo.processId);
    if (processInfo.handle == NULL) {
      DWORD error = GetLastError();
      logger(log, L"Unable to open WeaselServer handle [Error: " +
                      std::to_wstring(error) + L"][PID: " +
                      std::to_wstring(processInfo.processId) + L"]");
      processInfo = {};
    } else {
      TCHAR serverName[MAX_PATH];
      if (GetProcessImageFileName(processInfo.handle, serverName, MAX_PATH)) {
        if (endswith(serverName, weaselServerProcessName)) {
          logger(log, L"Opened WeaselServer handle [PID: " +
                          std::to_wstring(processInfo.processId) + L"][Name: " +
                          serverName + L"][" +
                          handleToString(processInfo.handle) + L"]");
        } else {
          logger(log, L"Opened WeaselServer handle [PID: " +
                          std::to_wstring(processInfo.processId) + L"][" +
                          handleToString(processInfo.handle) +
                          L"], but the Process Name is not right [" +
                          serverName + L"]");
          CloseHandle(processInfo.handle);
          processInfo = {};
        }
      } else {
        DWORD error = GetLastError();
        logger(log, L"Opened WeaselServer handle [PID: " +
                        std::to_wstring(processInfo.processId) + L"][" +
                        handleToString(processInfo.handle) +
                        L"], but unable to get the Process Name [Error: " +
                        std::to_wstring(error) + L"]");
        CloseHandle(processInfo.handle);
        processInfo = {};
      }
    }
  }
  return processInfo;
}

static void waitForServerToExit(const std::wstring& log,
                                ProcessInfo& processInfo) {
  if (processInfo.handle) {
    logger(log, L"Waiting for WeaselServer to exit [PID: " +
                    std::to_wstring(processInfo.processId) + L"]");
    DWORD ret = WaitForSingleObject(processInfo.handle, INFINITE);
    BOOL isHandleOwner =
        handleWaitForSingleObject(log, weaselServerProcessName, ret);
    if (isHandleOwner) {
      ReleaseMutex(processInfo.handle);
    }
    CloseHandle(processInfo.handle);
    processInfo = {};
  } else {
    Sleep(checkServerInterval);
  }
}

void WeaselTSF::_StartServerGuardian() {
  isGuardianStartedMutex.lock();
  if (!isGuardianStarted) {
    isGuardianStarted = TRUE;
    std::thread th([this]() {
      std::wstring log = L"[WeaselTSF][Guardian]";
      HANDLE hMutex = createMutex(log, weaselServerGuardianMutex);
      ProcessInfo processInfo{};
      BOOL isOwner = FALSE;
      while (true) {
        waitForServerToExit(log, processInfo);
        if (!isOwner) {
          if (hMutex) {
            logger(log, L"Waiting for [" +
                            std::wstring(weaselServerGuardianMutex) + L"]");
            DWORD ret = WaitForSingleObject(hMutex, INFINITE);
            isOwner =
                handleWaitForSingleObject(log, weaselServerGuardianMutex, ret);
            if (!isOwner)
              continue;
          } else {
            hMutex = createMutex(log, weaselServerGuardianMutex);
            continue;
          }
        }
        logger(log, L"Running");
        processInfo = _CheckServer(log);
        processInfo = openServerHandle(log, processInfo);
      }
    });
    th.detach();
  }
  isGuardianStartedMutex.unlock();
}

WeaselTSF::WeaselTSF() {
  _cRef = 1;

  _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;

  _dwTextEditSinkCookie = TF_INVALID_COOKIE;
  _dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
  _fTestKeyDownPending = FALSE;
  _fTestKeyUpPending = FALSE;

  _fCUASWorkaroundTested = _fCUASWorkaroundEnabled = FALSE;

  _cand = new CCandidateList(this);

  DllAddRef();
}

WeaselTSF::~WeaselTSF() {
  DllRelease();
}

STDAPI WeaselTSF::QueryInterface(REFIID riid, void** ppvObject) {
  if (ppvObject == NULL)
    return E_INVALIDARG;

  *ppvObject = NULL;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfTextInputProcessor))
    *ppvObject = (ITfTextInputProcessor*)this;
  else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    *ppvObject = (ITfTextInputProcessorEx*)this;
  else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    *ppvObject = (ITfThreadMgrEventSink*)this;
  else if (IsEqualIID(riid, IID_ITfTextEditSink))
    *ppvObject = (ITfTextEditSink*)this;
  else if (IsEqualIID(riid, IID_ITfTextLayoutSink))
    *ppvObject = (ITfTextLayoutSink*)this;
  else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    *ppvObject = (ITfKeyEventSink*)this;
  else if (IsEqualIID(riid, IID_ITfCompositionSink))
    *ppvObject = (ITfCompositionSink*)this;
  else if (IsEqualIID(riid, IID_ITfEditSession))
    *ppvObject = (ITfEditSession*)this;
  else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
    *ppvObject = (ITfThreadFocusSink*)this;
  else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    *ppvObject = (ITfDisplayAttributeProvider*)this;

  if (*ppvObject) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDAPI_(ULONG) WeaselTSF::AddRef() {
  return ++_cRef;
}

STDAPI_(ULONG) WeaselTSF::Release() {
  LONG cr = --_cRef;

  assert(_cRef >= 0);

  if (_cRef == 0)
    delete this;

  return cr;
}

STDAPI WeaselTSF::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) {
  return ActivateEx(pThreadMgr, tfClientId, 0U);
}

STDAPI WeaselTSF::Deactivate() {
  m_client.EndSession();

  _InitTextEditSink(com_ptr<ITfDocumentMgr>());

  _UninitThreadMgrEventSink();

  _UninitKeyEventSink();
  _UninitPreservedKey();

  _UninitLanguageBar();

  _UninitCompartment();

  _pThreadMgr = NULL;

  _tfClientId = TF_CLIENTID_NULL;

  _cand->Destroy();

  return S_OK;
}

STDAPI WeaselTSF::ActivateEx(ITfThreadMgr* pThreadMgr,
                             TfClientId tfClientId,
                             DWORD dwFlags) {
  com_ptr<ITfDocumentMgr> pDocMgrFocus;
  _activateFlags = dwFlags;

  _pThreadMgr = pThreadMgr;
  _tfClientId = tfClientId;

  if (!_InitThreadMgrEventSink())
    goto ExitError;

  if ((_pThreadMgr->GetFocus(&pDocMgrFocus) == S_OK) &&
      (pDocMgrFocus != NULL)) {
    _InitTextEditSink(pDocMgrFocus);
  }

  if (!_InitKeyEventSink())
    goto ExitError;

  // if (!_InitDisplayAttributeGuidAtom())
  //	goto ExitError;
  //	some app might init failed because it not provide DisplayAttributeInfo,
  // like some opengl stuff
  _InitDisplayAttributeGuidAtom();

  if (!_InitPreservedKey())
    goto ExitError;

  if (!_InitLanguageBar())
    goto ExitError;

  if (!_IsKeyboardOpen())
    _SetKeyboardOpen(TRUE);

  if (!_InitCompartment())
    goto ExitError;

  _EnsureServerConnected();
  _InitializeCompartment();
  _StartServerGuardian();
  return S_OK;

ExitError:
  Deactivate();
  return E_FAIL;
}

STDMETHODIMP WeaselTSF::OnSetThreadFocus() {
  return S_OK;
}
STDMETHODIMP WeaselTSF::OnKillThreadFocus() {
  _AbortComposition();
  return S_OK;
}

STDMETHODIMP WeaselTSF::OnActivated(REFCLSID clsid,
                                    REFGUID guidProfile,
                                    BOOL isActivated) {
  if (!IsEqualCLSID(clsid, c_clsidTextService)) {
    return S_OK;
  }

  if (isActivated) {
    _ShowLanguageBar(TRUE);
  } else {
    _DeleteCandidateList();
    _ShowLanguageBar(FALSE);
  }
  return S_OK;
}

void WeaselTSF::_ResetStatusAfterReConnected(const std::wstring& callFrom) {
  std::wstring log = L"[WeaselTSF][" + callFrom + L"][RestoreStatus]";
  DWORD changed_options = 0;
  DWORD changed_values = 0;
  if (_isBackupStatusInitialized && !_status.options.empty()) {
    DWORD new_options = std::stoi(_status.options);
    DWORD backup_options = std::stoi(_backupStatus.options);

    if (_backupStatus.option_list.compare(_status.option_list)) {
      logger(log, L"Option List Changed[" + _backupStatus.option_list +
                      L"] -> [" + _status.option_list + L"]");
      std::set<std::string> backup_option_list =
          split2set(ws2s(_backupStatus.option_list), ",");
      std::set<std::string> new_option_list =
          split2set(ws2s(_status.option_list), ",");
      _allOptions = new_option_list;

      backup_options =
          updateOptionValueByNewList(log + L"[Options]", backup_options,
                                     backup_option_list, new_option_list);
      _backupStatus.options = std::to_wstring(backup_options);
    }

    if (_backupStatus.schema_id.compare(_status.schema_id)) {
      int schemaIndex =
          indexOf(_status.schema_list, _backupStatus.schema_id, L",");
      if (schemaIndex >= 0) {
        logger(log, L"Schema Id[" + _backupStatus.schema_id + L"][" +
                        std::to_wstring(schemaIndex) + L"][" +
                        _status.schema_list + L"]");
        _status.schema_id = _backupStatus.schema_id;
        m_client.SelectSchema(schemaIndex);
      } else {
        logger(log, L"Schema Id not exits[" + _backupStatus.schema_id + L"][" +
                        _status.schema_list + L"]");
        _backupStatus.schema_id = _status.schema_id;
      }
    }
    if (_backupStatus.ascii_mode != _status.ascii_mode) {
      _status.ascii_mode = _backupStatus.ascii_mode;
    }
    if (_backupStatus.full_shape != _status.full_shape) {
      _status.full_shape = _backupStatus.full_shape;
    }

    int i = 0;
    std::set<std::string>::iterator it;
    for (it = _allOptions.begin(); it != _allOptions.end(); ++it) {
      std::string option_name = *it;
      bool status_option = bit_check(new_options, i);
      bool backup_option = bit_check(backup_options, i);
      if (backup_option != status_option) {
        changed_options = bit_set(changed_options, i);
        logger(log, s2ws(option_name));
        if (backup_option)
          changed_values = bit_set(changed_values, i);
      }
      i++;
    }
    if (changed_options) {
      logger(log, L"Backup Options[" + tobitstr(backup_options) + L"]");
      logger(log, L"Set Options[" + tobitstr(changed_options) + L"] -> [" +
                      tobitstr(changed_values) + L"]");
      m_client.SetOptions(changed_options, changed_values);
    }
    _status.options = _backupStatus.options;
    _backupStatus.option_list = _status.option_list;
  }
  _UpdateLanguageBar(callFrom + L"_ResetStatus", _status);
}

void WeaselTSF::_EnsureServerConnected() {
  if (!m_client.Echo()) {
    m_client.Disconnect();
    m_client.Connect(NULL);
    m_client.StartSession();
    weasel::ResponseParser parser(NULL, NULL, &_status, NULL, &_cand->style());
    bool ok = m_client.GetResponseData(std::ref(parser));
    if (ok) {
      isFirstEnsureMutex.lock();
      if (isFirstEnsure) {
        isFirstEnsure = FALSE;
        _UpdateLanguageBar(L"_EnsureServer", _status);
      } else {
        _ResetStatusAfterReConnected(L"_EnsureServer_1");
      }
      isFirstEnsureMutex.unlock();
    }
    if (!m_client.Echo()) {
      _CheckServerOnEnsure();

      weasel::ResponseParser parser(NULL, NULL, &_status, NULL,
                                    &_cand->style());
      bool ok = m_client.GetResponseData(std::ref(parser));
      if (ok) {
        _ResetStatusAfterReConnected(L"_EnsureServer_2");
      }
    }
  }
}
