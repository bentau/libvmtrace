#ifndef __LINUX_VM_H
#define __LINUX_VM_H

#include "libvmtrace.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <chrono>

using namespace std;

//class LinuxVM;
// class SyscallProcessor;

enum SyscallType 
{ 
	ALL_SYSCALLS, BEFORE_CALL, AFTER_CALL 
};

enum CodeInjectionType
{
	PAGE_FAULT, FORK_EXEC, NOPE
};

struct CodeInjection
{
	CodeInjectionType type;

	vmi_pid_t target_pid;
	addr_t target_dtb;
	addr_t task_struct;

	vector<addr_t> target_addrs;
	vector<pair<addr_t, addr_t>> result_addrs;

	addr_t entry_addr;

	addr_t breakpoint1;
	addr_t breakpoint_pa1;

	addr_t breakpoint2;
	addr_t breakpoint_pa2;

	addr_t breakpoint3;
	addr_t breakpoint_pa3;

	char* saved_code;
	char* inject_code;
	char* tmp;

	size_t instr_size;
	size_t total_page_text;

	EventListener* evl;

	ProcessBreakpointEvent* bp1;
	ProcessBreakpointEvent* bp2;
	ProcessBreakpointEvent* bp3;

	x86_registers_t regs;

	string command;

	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> start1;
};

// struct CodeInjection
// {
// 	vmi_pid_t target_pid;
// 	addr_t target_dtb;
// 	addr_t task_struct;

// 	ProcessBreakpointEvent* bp1;
// 	ProcessBreakpointEvent* bp2;
// 	ProcessBreakpointEvent* bp3;

// 	char* saved_code;
// 	size_t instr_size;

// 	reg_t rax;
// 	reg_t rdi;
// 	reg_t rsi;
// 	reg_t rdx;
// };

class SyscallBreakpoint: public BreakpointEvent 
{
	public:
		SyscallBreakpoint(addr_t addr, EventListener& el, int nr, SyscallType type, bool is32bit, bool processJson):
							BreakpointEvent("syscall_"+to_string(nr)+(type == AFTER_CALL ? " after call" : ""),addr, el),
							_nr(nr),
							_type(type),
							_is32bit(is32bit),
							_processJson(processJson),
							_syscall(nullptr) {}

		SyscallBreakpoint(addr_t addr, EventListener& el, int nr, SyscallType type, bool is32bit, bool processJson, SyscallBasic* s):
							BreakpointEvent("syscall_"+to_string(nr)+(type == AFTER_CALL ? " after call" : ""),addr, el),
							_nr(nr),
							_type(type),
							_is32bit(is32bit),
							_processJson(processJson),
							_syscall(s) {}

		~SyscallBreakpoint() {}
		inline int GetNr() const { return _nr; }
		inline SyscallType GetType() const { return _type; }
		inline SyscallBasic* GetSyscall() const { return _syscall; }
		inline bool Is32bit() const { return _is32bit; }
		inline bool ProcessJson() const { return _processJson; }

	private:
		int _nr;
		SyscallType _type;
		bool _is32bit;
		bool _processJson;
		SyscallBasic* _syscall;
};

struct OpenFile
{
	string path;
	uint64_t fd;
	uint32_t mode;
	bool write;
};

class SyscallProcessor : public EventListener
{
	public:
		SyscallProcessor(LinuxVM* lvm) : _lvm(lvm) {}
		bool callback(const Event* ev, void* data);
	private:
		LinuxVM* _lvm;
};

class CodeInjectionProcessorCr3 : public EventListener
{
	public:
		CodeInjectionProcessorCr3(LinuxVM* lvm) : _lvm(lvm) {}
		bool callback(const Event* ev, void* data);
	private:
		LinuxVM* _lvm;
};

class CodeInjectionProcessorInt3 : public EventListener
{
	public:
		CodeInjectionProcessorInt3(LinuxVM* lvm) : _lvm(lvm) {}
		bool callback(const Event* ev, void* data);
	private:
		LinuxVM* _lvm;
};

class LinuxVM : public OperatingSystem 
{
	public:
		LinuxVM(SystemMonitor* sm);

		vector<Process> GetProcessList();
		vector <NetworkConnection> GetNetworkConnections(const Process& p, const ConnectionType type);
		vector <OpenFile> GetOpenFiles(const Process& p);
		vector<vm_area> GetMMaps(const Process& p);

		status_t RegisterSyscall(SyscallEvent& ev);
		status_t DeRegisterSyscall(SyscallEvent& ev);
		bool ProcessSyscall(const SyscallBreakpoint* ev, void* data, vmi_instance_t);

		status_t PauseSyscall(SyscallEvent& ev);
		status_t ResumeSyscall(SyscallEvent& ev);

		status_t RegisterProcessChange(ProcessChangeEvent& ev);
		status_t DeRegisterProcessChange(ProcessChangeEvent& ev);

		SystemMonitor* GetSystemMonitor() const { return _sm; }

		void Stop();

		addr_t GetSymbolAddrVa(const string binaryPath, const Process& p, const string symbolName);
		addr_t GetSymbolAddrPa(const string binaryPath, const Process& p, const string symbolName);
		
		void PopulatePageFaultAdress(const vmi_pid_t pid, const addr_t target, EventListener* evl);
		status_t InvokePageFault(uint64_t total_address_per_inst);

		status_t InvokeCommand(const vmi_pid_t pid, string command, EventListener* evl);

		bool ProcessCR3CodeInjection(vmi_instance_t vmi, vmi_event_t *event);
		bool ProcessInt3CodeInjection(const ProcessBreakpointEvent* ev, void* data, vmi_instance_t vmi);

		pair<addr_t, addr_t> GetCodeArea(vmi_pid_t pid);
	private:
		addr_t _tgid_offset, _name_offset, _mm_offset, _tasks_offset, _parent_offset, _pgd_offset, _real_cred_offset, _uid_offset, _fs_offset, _pwd_offset;
		addr_t _dentry_d_name_offset, _dentry_parent_offset;
		addr_t _files_offset, _fdt_offset, _fd_offset, _f_mode_offset, _f_path_offset;
		addr_t _private_data_offset, _sk_offset, _u1_offset, _u3_offset;
		addr_t _vm_end_offset, _vm_flags_offset, _vm_file_offset, _vm_pgoff_offset, _vm_next_offset, _exe_file_offset;
		addr_t _code_start_offset, _code_end_offset;

		addr_t _thread_struct_offset, _sp_offset, _sp0_offset, _sp_on_pt_regs_offset, _ip_on_pt_regs_offset, _current_task_offset;
		addr_t _socket_type_offset, _socket_family;

		string d_path(addr_t path, vmi_instance_t vmi);
		uint32_t create_path(addr_t dentry, char* buf, vmi_instance_t vmi);

		addr_t GetSyscallAddrVA(unsigned int syscall_nr, bool is32bit, vmi_instance_t vmi);
		addr_t GetSyscallAddrPA(unsigned int syscall_nr, bool is32bit, vmi_instance_t vmi);

		status_t InvokeCodeInjection(CodeInjectionType type, const vmi_pid_t pid, const addr_t target, string command, EventListener* evl);

		EventManager<int, const SyscallEvent*> _SyscallEvents64;
		EventManager<int, const SyscallEvent*> _SyscallEvents32;
		map<int, const SyscallBreakpoint> _Syscallbps64;
		map<int, const SyscallBreakpoint> _Syscallbps32;
		SyscallProcessor _syscallProc;

		uint64_t _total_address_per_inst;

		ElfHelper* _eh;
		string _binaryPathTemp;
		char* _binaryMap;

		// vmi_event_t _cr3_event;
		ProcessChangeEvent* _process_change;
		vector<CodeInjection> _code_injections;
		CodeInjectionProcessorCr3 _code_injection_proc_cr3;
		CodeInjectionProcessorInt3 _code_injection_proc_int3;
};
#endif