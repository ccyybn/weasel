#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <map>
#include <string>

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
typedef std::map<std::string, AppOptions, CaseInsensitiveCompare> AppOptionsByAppName;
struct SessionStatus
{
	SessionStatus() : style(weasel::UIStyle()), __synced(false) { RIME_STRUCT(RimeStatus, status); }
	weasel::UIStyle style;
	RimeStatus status;
	bool __synced;
};
typedef std::map<RimeSessionId, SessionStatus> SessionStatusMap;
class RimeWithWeaselHandler :
	public weasel::RequestHandler
{
public:
	RimeWithWeaselHandler(weasel::UI *ui);
	virtual ~RimeWithWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual RimeSessionId FindSession(RimeSessionId session_id);
	virtual RimeSessionId AddSession(LPWSTR buffer, EatLine eat = 0);
	virtual RimeSessionId RemoveSession(RimeSessionId session_id);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, RimeSessionId session_id, EatLine eat);
	virtual void CommitComposition(RimeSessionId session_id);
	virtual void ClearComposition(RimeSessionId session_id);
	virtual void SelectCandidateOnCurrentPage(size_t index, RimeSessionId session_id);
	virtual void FocusIn(DWORD param, RimeSessionId session_id);
	virtual void FocusOut(DWORD param, RimeSessionId session_id);
	virtual void UpdateInputPosition(RECT const& rc, RimeSessionId session_id);
	virtual void StartMaintenance();
	virtual void EndMaintenance();
	virtual void SetOption(RimeSessionId session_id, const std::string &opt, bool val);
	virtual void UpdateColorTheme(BOOL darkMode);

	void OnUpdateUI(std::function<void()> const &cb);

private:
	void _Setup();
	bool _IsDeployerRunning();
	void _UpdateUI(RimeSessionId session_id);
	void _LoadSchemaSpecificSettings(RimeSessionId session_id, const std::string& schema_id);
	void _LoadAppInlinePreeditSet(RimeSessionId session_id, bool ignore_app_name = false);
	bool _ShowMessage(weasel::Context& ctx, weasel::Status& status);
	bool _Respond(RimeSessionId session_id, EatLine eat);
	void _ReadClientInfo(RimeSessionId session_id, LPWSTR buffer);
	void _GetCandidateInfo(weasel::CandidateInfo &cinfo, RimeContext &ctx);
	void _GetStatus(weasel::Status &stat, RimeSessionId session_id, weasel::Context& ctx);
	void _GetContext(weasel::Context &ctx, RimeSessionId session_id);
	void _UpdateShowNotifications(RimeConfig* config, bool initialize = false);

	bool _IsSessionTSF(RimeSessionId session_id);
	void _UpdateInlinePreeditStatus(RimeSessionId session_id);

	AppOptionsByAppName m_app_options;
	weasel::UI* m_ui;  // reference
	RimeSessionId m_active_session;
	bool m_disabled;
	std::string m_last_schema_id;
	std::string m_last_app_name;
	weasel::UIStyle m_base_style;
	std::map<std::string, bool> m_show_notifications;
	std::map<std::string, bool> m_show_notifications_base;
	std::function<void()> _UpdateUICallback;

	static void OnNotify(void* context_object,
		                 RimeSessionId session_id,
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
};
