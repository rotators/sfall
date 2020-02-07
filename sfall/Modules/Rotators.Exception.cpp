#include <cstdint>

#include <DbgHelp.h>
#include <PsApi.h>
#include <TlHelp32.h>

#include "../Logging.h"

#include "Rotators.h"
#include "Rotators.Exception.h"

#pragma comment(lib, "DbgHelp.lib")

using namespace rfall;

// for easy (re)align, random naming
#define STR_APP        "Application\n"
#define STR_APP_OS     "\tOS       %d.%d.%d (%s)\n"

#define STR_EX         "Exception\n"
#define STR_EX_CODE    "\tCode     "
#define STR_EX_ADDRESS "\tAddress  0x%p\n"
#define STR_EX_FLAGS   "\tFlags    0x%x\n"
#define STR_EX_READ    "\tInfo     Attempted to read to an 0x%p"
#define STR_EX_WRITE   "\tInfo     Attempted to write to an 0x%p"
#define STR_EX_EXECPR  "\tInfo     Data execution prevention to an 0x%p"
#define STR_EX_PARAM   "\tInfo     %u    0x%p\n"

// Old version of the structure, used before Vista
typedef struct _IMAGEHLP_MODULE64_V2 {
	DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
	DWORD64  BaseOfImage;            // base load address of module
	DWORD    ImageSize;              // virtual size of the loaded module
	DWORD    TimeDateStamp;          // date/time stamp from pe header
	DWORD    CheckSum;               // checksum from the pe header
	DWORD    NumSyms;                // number of symbols in the symbol table
	SYM_TYPE SymType;                // type of symbols loaded
	CHAR     ModuleName[32];         // module name
	CHAR     ImageName[256];         // image name
	CHAR     LoadedImageName[256];   // symbol file name
} IMAGEHLP_MODULE64_V2;

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ex) {

	// TODO timestamp as part of filename
	// TODO dedicated directory
	FILE* f = fopen("CRASH.TXT", "wt");
	if(f) {
		fprintf(f, STR_APP);

		// TODO versions info (fallout, sfall, HRP)

		// TODO human-friendly OS name
		// TODO "GetVersionEx was declared deprecated" warning
		OSVERSIONINFOA ver;
		memset(&ver, 0, sizeof(OSVERSIONINFOA));
		ver.dwOSVersionInfoSize = sizeof(ver);
		if(GetVersionEx((OSVERSIONINFOA*)&ver))
			fprintf(f, STR_APP_OS, ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber, ver.szCSDVersion);

		// TODO timestamp

		fprintf(f,"\n");

		if(ex) {
			fprintf(f, STR_EX);
			fprintf(f, STR_EX_CODE);
			switch(ex->ExceptionRecord->ExceptionCode) {
				#define CASE_EXCEPTION(e) case e: fprintf(f, "%s (0x%x)", #e, e); break;
				CASE_EXCEPTION(EXCEPTION_ACCESS_VIOLATION);
				CASE_EXCEPTION(EXCEPTION_DATATYPE_MISALIGNMENT);
				CASE_EXCEPTION(EXCEPTION_BREAKPOINT);
				CASE_EXCEPTION(EXCEPTION_SINGLE_STEP);
				CASE_EXCEPTION(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
				CASE_EXCEPTION(EXCEPTION_FLT_DENORMAL_OPERAND);
				CASE_EXCEPTION(EXCEPTION_FLT_DIVIDE_BY_ZERO);
				CASE_EXCEPTION(EXCEPTION_FLT_INEXACT_RESULT);
				CASE_EXCEPTION(EXCEPTION_FLT_INVALID_OPERATION);
				CASE_EXCEPTION(EXCEPTION_FLT_OVERFLOW);
				CASE_EXCEPTION(EXCEPTION_FLT_STACK_CHECK);
				CASE_EXCEPTION(EXCEPTION_FLT_UNDERFLOW);
				CASE_EXCEPTION(EXCEPTION_INT_DIVIDE_BY_ZERO);
				CASE_EXCEPTION(EXCEPTION_INT_OVERFLOW);
				CASE_EXCEPTION(EXCEPTION_PRIV_INSTRUCTION);
				CASE_EXCEPTION(EXCEPTION_IN_PAGE_ERROR);
				CASE_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION);
				CASE_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION);
				CASE_EXCEPTION(EXCEPTION_STACK_OVERFLOW);
				CASE_EXCEPTION(EXCEPTION_INVALID_DISPOSITION);
				CASE_EXCEPTION(EXCEPTION_GUARD_PAGE);
				CASE_EXCEPTION(EXCEPTION_INVALID_HANDLE);
				default:
					fprintf(f, "0x%x", ex->ExceptionRecord->ExceptionCode);
					break;
			}
			fprintf(f, "\n");
			fprintf(f, STR_EX_ADDRESS, ex->ExceptionRecord->ExceptionAddress);
			fprintf(f, STR_EX_FLAGS, ex->ExceptionRecord->ExceptionFlags);
			if(ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ex->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
				int readWrite = (int)ex->ExceptionRecord->ExceptionInformation[0];
				if(readWrite == 0)
					fprintf(f, STR_EX_READ, (void*)ex->ExceptionRecord->ExceptionInformation[1]);
				else if(readWrite == 1)
					fprintf(f, STR_EX_WRITE, (void*)ex->ExceptionRecord->ExceptionInformation[1]);
				else // readWrite == 8
					fprintf(f, STR_EX_EXECPR, (void*)ex->ExceptionRecord->ExceptionInformation[1]);
				if(ex->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
					fprintf(f, ", NTSTATUS %p", (void*)ex->ExceptionRecord->ExceptionInformation[2]);
				fprintf(f, "\n");
			}
			else {
				for(DWORD i = 0; i < ex->ExceptionRecord->NumberParameters; i++)
					fprintf(f, STR_EX_PARAM, i, (void*)ex->ExceptionRecord->ExceptionInformation[i]);
			}

			fprintf(f, "\n");
		} // if(ex)

		// Collect current threads
		HANDLE   process = GetCurrentProcess();
		DWORD    threads_ids[1024] = { GetCurrentThreadId() };
		uint32_t threads_ids_count = 1;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if(snapshot != INVALID_HANDLE_VALUE) {
			THREADENTRY32 te;
			te.dwSize = sizeof(te);
			if(Thread32First(snapshot, &te)) {
				while(true) {
					if(te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) {
						if(te.th32OwnerProcessID == GetCurrentProcessId() && te.th32ThreadID != threads_ids[0])
							threads_ids[threads_ids_count++] = te.th32ThreadID;
					}

					te.dwSize = sizeof(te);
					if(!Thread32Next(snapshot, &te))
						break;
				}
			}

			CloseHandle(snapshot);
		}
		else
			fprintf(f, "CreateToolhelp32Snapshot fail\n");

		// Init symbols
		BOOL symInit = SymInitialize(process, NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);

		// Print information about each thread
		for(uint32_t i = 0; i < threads_ids_count; i++) {
			DWORD       tid = threads_ids[i];
			HANDLE      t = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT, FALSE, tid);
			const char* tname = nullptr; // [FOnline] Thread::FindName(tid);
			fprintf(f, "Thread '%s' (%u%s)\n", tname ? tname : "Unknown", tid, !i ? ", current" : "");

			CONTEXT context;
			memset(&context, 0, sizeof(context));
			context.ContextFlags = CONTEXT_FULL;

			if(tid == GetCurrentThreadId()) {
				if(ex)
					memcpy(&context, ex->ContextRecord, sizeof(CONTEXT));
				else {
					__asm label : __asm mov[context.Ebp], ebp;
					__asm mov[context.Esp], esp;
					__asm mov eax, [label];
					__asm mov[context.Eip], eax;
				}
			}
			else {
				SuspendThread(t);
				GetThreadContext(t, &context);
				ResumeThread(t);
			}

			STACKFRAME64 stack;
			memset(&stack, 0, sizeof(stack));

			DWORD machine_type = IMAGE_FILE_MACHINE_I386;
			stack.AddrFrame.Mode = AddrModeFlat;
			stack.AddrFrame.Offset = context.Ebp;
			stack.AddrPC.Mode = AddrModeFlat;
			stack.AddrPC.Offset = context.Eip;
			stack.AddrStack.Mode = AddrModeFlat;
			stack.AddrStack.Offset = context.Esp;

			#define STACKWALK_MAX_NAMELEN    (1024)
			char symbol_buffer[sizeof(SYMBOL_INFO) + STACKWALK_MAX_NAMELEN];
			SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buffer;
			memset(symbol, 0, sizeof(SYMBOL_INFO) + STACKWALK_MAX_NAMELEN);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = STACKWALK_MAX_NAMELEN;

			struct RPM {
				static BOOL __stdcall Call(HANDLE hProcess, DWORD64 qwBaseAddress, PVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead) {
					SIZE_T st;
					BOOL   bRet = ReadProcessMemory(hProcess, (LPVOID)qwBaseAddress, lpBuffer, nSize, &st);
					      *lpNumberOfBytesRead = (DWORD)st;
					return bRet;
				}
			};

			int frame_num = 0;
			while(StackWalk64(machine_type, process, t, &stack, &context, RPM::Call, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
				struct CSE {
					struct CallstackEntry {
						DWORD64 offset;
						CHAR    name[STACKWALK_MAX_NAMELEN];
						CHAR    undName[STACKWALK_MAX_NAMELEN];
						CHAR    undFullName[STACKWALK_MAX_NAMELEN];
						DWORD64 offsetFromSmybol;
						DWORD   offsetFromLine;
						DWORD   lineNumber;
						CHAR    lineFileName[STACKWALK_MAX_NAMELEN];
						DWORD   symType;
						LPCSTR  symTypeString;
						CHAR    moduleName[STACKWALK_MAX_NAMELEN];
						DWORD64 baseOfImage;
						CHAR    loadedImageName[STACKWALK_MAX_NAMELEN];
					};

					static void OnCallstackEntry(FILE* fd, int frame_num, CallstackEntry& entry) {
						if(frame_num >= 0 && entry.offset != 0) {
							if(entry.name[0] == 0)
								strcpy_s(entry.name, "(function-name not available)");
							if(entry.undName[0] != 0)
								strcpy_s(entry.name, entry.undName);
							if(entry.undFullName[0] != 0)
								strcpy_s(entry.name, entry.undFullName);
							if(entry.moduleName[0] == 0)
								strcpy_s(entry.moduleName, "???");

							fprintf(fd, "\t[0x%08x] ", static_cast<uint32_t>(entry.offset));
							// TODO there is something seriously broken here, any change to format can break arguments
							fprintf(fd, "%s, %s + 0x%x", entry.moduleName, entry.name, static_cast<uint32_t>(entry.offsetFromSmybol));
							if(entry.lineFileName[0] != 0)
								fprintf(fd, ", %s (%d)\n", entry.lineFileName, entry.lineNumber);
							else
								fprintf(fd, "\n");
						}
					}
				};

				CSE::CallstackEntry callstack;
				memset(&callstack, 0, sizeof(callstack));
				callstack.offset = stack.AddrPC.Offset;

				IMAGEHLP_LINE64 line;
				memset(&line, 0, sizeof(line));
				line.SizeOfStruct = sizeof(line);

				if(stack.AddrPC.Offset == stack.AddrReturn.Offset) {
					static const uint8_t frame_limit = 255;
					if(frame_num >= frame_limit) {
						fprintf(f, "\tEndless callstack!\n");
						break;
					}
				}

				if(stack.AddrPC.Offset != 0) {
					if(SymFromAddr(process, stack.AddrPC.Offset, &callstack.offsetFromSmybol, symbol)) {
						strcpy_s(callstack.name, symbol->Name);
						UnDecorateSymbolName(symbol->Name, callstack.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY);
						UnDecorateSymbolName(symbol->Name, callstack.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE);
					}

					if(SymGetLineFromAddr64(process, stack.AddrPC.Offset, &callstack.offsetFromLine, &line)) {
						callstack.lineNumber = line.LineNumber;
						strcpy_s(callstack.lineFileName, line.FileName);
						// [FOnline] FileManager::ExtractFileName(callstack.lineFileName, callstack.lineFileName);
					}

					IMAGEHLP_MODULE64 module;
					memset(&module, 0, sizeof(IMAGEHLP_MODULE64));
					module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
					if(SymGetModuleInfo64(process, stack.AddrPC.Offset, &module) ||
						(module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2),
						 SymGetModuleInfo64(process, stack.AddrPC.Offset, &module))) {
						switch(module.SymType) {
							case SymNone:
								callstack.symTypeString = "-nosymbols-";
								break;
							case SymCoff:
								callstack.symTypeString = "COFF";
								break;
							case SymCv:
								callstack.symTypeString = "CV";
								break;
							case SymPdb:
								callstack.symTypeString = "PDB";
								break;
							case SymExport:
								callstack.symTypeString = "-exported-";
								break;
							case SymDeferred:
								callstack.symTypeString = "-deferred-";
								break;
							case SymSym:
								callstack.symTypeString = "SYM";
								break;
								#if API_VERSION_NUMBER >= 9
							case SymDia:
								callstack.symTypeString = "DIA";
								break;
								#endif
							case SymVirtual:
								callstack.symTypeString = "Virtual";
								break;
							default:
								callstack.symTypeString = NULL;
								break;
						}

						strcpy_s(callstack.moduleName, module.ModuleName);
						callstack.baseOfImage = module.BaseOfImage;
						strcpy_s(callstack.loadedImageName, module.LoadedImageName);
					}
				}

				CSE::OnCallstackEntry(f, frame_num, callstack);
				if(stack.AddrReturn.Offset == 0)
					break;

				frame_num++;
			}

			fprintf(f, "\n");
			CloseHandle(t);
		}

		SymCleanup(process);
		CloseHandle(process);
		fclose(f);
	} // if(f)

	//return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_SEARCH;
}

void Exception::init() {
	if(ini.GetBool("Debugging", "CatchExceptions", false)) {
		sfall::dlog_f("> enabled\n", DL_INIT);
		SetUnhandledExceptionFilter(ExceptionHandler);
	}
}
