#ifndef __PROCESS_CHANGE_LOGGER_
#define __PROCESS_CHANGE_LOGGER_

#include <libvmtrace.hpp>

class ProcessChangeLogger: public Plugin, public EventListener {
	public:
		ProcessChangeLogger(std::string vm_id, OperatingSystem& os, Log& log): _os(os), _log(log), _log_name(vm_id+"procchange"), _vm_id(vm_id) {}
		const std::string ExecuteCommand(const std::string command, 
				const std::vector<std::string> par,
				const std::string command_id,
				const std::string vm_id) ;
		const std::vector<std::string> GetListCommands(void) const {
			return std::vector<std::string>();
		}
		const std::string GetName(void) const  { return "ProcessChangeLogger"; }
		const void Stop(void) { }
		bool callback(const Event* ev, void* data) {
			StringBuffer strbuf;
		        Writer<StringBuffer> writer(strbuf);
			writer.StartObject();
			writer.Key("old");
			writer.String("0");
		        writer.EndObject();

			_log.log(_vm_id, _log_name, strbuf.GetString());	

			return false;
		}
	private:
		OperatingSystem& _os;
		ProcessChangeEvent* _pce;
		Log& _log;
		std::string _log_name;
		std::string _vm_id;
};
#endif
