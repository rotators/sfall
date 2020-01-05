#include <string>
#include <vector>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Rotators.Sandbox.h"
#include "Rotators.h"
using namespace rfall;

namespace sfall
{

	// FUNC(xenumfiles_, 0x4DFB3C)
	// Function for enumerating files in filesystem and loaded DAT files based on a searchstring.
	// Not sure if this is helpful or just a waste of time... this should be rewritten in c++.
	// searchstring can be: "PROTO\\CRITTERS\\*.PRO" for example.
	// eax = searchstring
	// edx = buffer for storing the result
	// xenum_files(char* searchstring, void* result);
	static void _declspec(naked) _xenum_files()
	{
		__asm {
			push ecx
			push esi
			push edi
			push ebp
			sub esp, 0x730
			mov dword ptr ss : [esp + 0x724] , eax // searchstring
			mov dword ptr ss : [esp + 0x728] , edx // result buffer
			mov esi, ebx
			test eax, eax
			jne search_not_null
			mov ecx, 0x34D
			mov ebx, 0x50fc40 // aXfile_c, FILE.C
			mov edx, 0x50fc9c // aFilespec 
			call fo::funcoffs::assert_
		search_not_null:
			cmp dword ptr ss : [esp + 0x728], 0
		jne result_buffer_not_null
			mov ecx, 0x34E
			mov ebx, 0x50FC40 // aXfile_c, FILE.C
			mov edx, 0x50FC88 // aEnumfunc
			xor eax, eax
			call fo::funcoffs::assert_
		result_buffer_not_null:
			lea eax, dword ptr ss : [esp + 0x51C]
			lea ecx, dword ptr ss : [esp + 0x41C]
			lea ebx, dword ptr ss : [esp + 0x61C]
			lea edx, dword ptr ss : [esp + 0x72C] // ?
			push eax
			mov eax, dword ptr ss : [esp + 0x728]
			mov dword ptr ss : [esp + 0x318], esi
			call fo::funcoffs::_splitpath_
			cmp byte ptr ss : [esp + 0x72C], 0
			jne find_first // if ? 
			mov dl, byte ptr ss : [esp + 0x61C]
			cmp dl, 0x5C // '\\'
			je find_first
			cmp dl, 0x2F // '/'
			je find_first
			cmp dl, 0x2E // '.'
			jne read_data_folder
		find_first:
			lea edx, dword ptr ss : [esp + 0x71C] // local path??
			mov eax, dword ptr ss : [esp + 0x724] // searchstring
			call fo::funcoffs::xsys_findfirst_
			test eax, eax
			je ret_close
			mov ebp, 0x50FCA8 // __11
			mov edi, 0x50FCAC // a__
			mov esi, 0x1E
		loop_compare_files:
			xor bh, bh
			mov eax, dword ptr ss : [esp + 0x720]
			mov byte ptr ss : [esp + 0x310] , bh
			test byte ptr ds : [eax + 0x15] , 10
			je make_path
			mov edx, ebp
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_find_next
			mov eax, dword ptr ss : [esp + 0x720]
			mov edx, edi
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_find_next
			mov byte ptr ss : [esp + 0x310] , 1
		make_path:
			push 0
			mov ecx, dword ptr ss : [esp + 0x724]
			lea ebx, dword ptr ss : [esp + 0x620]
			lea edx, dword ptr ss : [esp + 0x730]
			lea eax, dword ptr ss : [esp + 0x210]
			add ecx, esi
			call fo::funcoffs::_makepath_
			lea eax, dword ptr ss : [esp + 0x20C] // current file
			push eax
			call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
			add esp, 4
			test eax, eax
			je ret_close
		fs_find_next:
			lea eax, dword ptr ss : [esp + 0x71C]
			call fo::funcoffs::xsys_findnext_
			test eax, eax
			jne loop_compare_files
			jmp ret_close
		read_data_folder:
			mov ebp, dword ptr ds : [FO_VAR_paths] // /data/
			test ebp, ebp
			je split_path
		process_path:
			test byte ptr ss : [ebp + 8], 1       // 1 = is dat
			je search_on_fs                       // jump if not
			mov ebx, dword ptr ss : [esp + 0x724] // search pattern
			mov edx, esp                          // file to look for
			mov eax, dword ptr ss : [ebp + 4]     // dat to look in
			call fo::funcoffs::dbase_findfirst_
			test eax, eax
			je db_close
			mov byte ptr ss : [esp + 0x310] , 2
		db_process_file:
			mov esi, esp
			lea edi, dword ptr ss : [esp + 0x20C]
			push edi
		db_process_file_loop:
			mov al, byte ptr ds : [esi]
			mov byte ptr ds : [edi] , al
			cmp al, 0
			je db_process_file_done
			mov al, byte ptr ds : [esi + 1]
			add esi, 2
			mov byte ptr ds : [edi + 1], al
			add edi, 2
			cmp al, 0
			jne db_process_file_loop
		db_process_file_done:
			pop edi
			lea eax, dword ptr ss : [esp + 0x20C]
			push eax
			// call dword ptr ss : [esp + 0x72C]
			call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
			add esp, 4
			test eax, eax
			jne dat_next
			mov edx, esp
		db_close:
			mov eax, dword ptr ss : [ebp + 4]   // dat name
			call fo::funcoffs::dbase_findclose_ // close
			jmp ret_                            // return
		dat_next:
			mov edx, esp                       // current file
			mov eax, dword ptr ss : [ebp + 4]  // dat name
			call fo::funcoffs::dbase_findnext_
			test eax, eax                     
			jne db_process_file
			mov edx, esp
			mov eax, dword ptr ss : [ebp + 4]  // dat name
			call fo::funcoffs::dbase_findclose_
			jmp check_next_path
		search_on_fs:
			mov ecx, dword ptr ss : [esp + 0x724]
			push ecx
			mov esi, dword ptr ss : [ebp]
			push esi
			push 0x50fc64 // SS_14, no idea what this is.
			lea eax, dword ptr ss : [esp + 0x324]
			push eax
			call fo::funcoffs::sprintf_
			add esp, 0x10
			lea edx, dword ptr ss : [esp + 0x71C]
			lea eax, dword ptr ss : [esp + 0x318]
			call fo::funcoffs::xsys_findfirst_
			test eax, eax
			je fs_find_close
			mov edi, 0x50FCAC // a__, no idea what this is.
			mov esi, 0x1E
		fs_compare_files2:
			xor bh, bh
			mov eax, dword ptr ss : [esp + 0x720]
			mov byte ptr ss : [esp + 0x310] , bh
			test byte ptr ds : [eax + 0x15] , 0x10
			je fs_make_path
			mov edx, 0x50FCA8 // __11, no idea what this is.
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_findnext2
			mov eax, dword ptr ss : [esp + 0x720]
			mov edx, edi
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_find_close
			mov byte ptr ss : [esp + 0x310] , 1
		fs_make_path:
			push 0
			mov ecx, dword ptr ss : [esp + 0x724]
			lea ebx, dword ptr ss : [esp + 0x620]
			lea edx, dword ptr ss : [esp + 0x730]
			lea eax, dword ptr ss : [esp + 0x210]
			add ecx, esi
			call fo::funcoffs::_makepath_
			lea eax, dword ptr ss : [esp + 0x20C]
			push eax
			//call dword ptr ss : [esp + 0x72C]
			call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
			add esp, 4
			test eax, eax
			je ret_close
		fs_findnext2:
			lea eax, dword ptr ss : [esp + 0x71C]
			call fo::funcoffs::xsys_findnext_
			test eax, eax
			jne fs_compare_files2
		fs_find_close:
			lea eax, dword ptr ss : [esp + 0x71C]
			call fo::funcoffs::xsys_findclose_
		check_next_path:
			mov ebp, dword ptr ss : [ebp + 0xC] // next path
			test ebp, ebp                       // if one exists
			jne process_path
		split_path:
			lea eax, dword ptr ss : [esp + 0x51C] // file extension
			lea ecx, dword ptr ss : [esp + 0x41C] // ?
			lea ebx, dword ptr ss : [esp + 0x61C] // directory
			push eax
			lea edx, dword ptr ss : [esp + 0x730] // ??
			mov eax, dword ptr ss : [esp + 0x728] // searchstring
			call fo::funcoffs::_splitpath_
			lea edx, dword ptr ss : [esp + 0x71C]
			mov eax, dword ptr ss : [esp + 0x724]
			call fo::funcoffs::xsys_findfirst_
			test eax, eax
			je ret_close
			mov ebp, 0x50FCA8 // __11
			mov edi, 0x50FCAC // a__
			mov esi, 0x1E
		fs_compare_files:
			xor dl, dl
			mov eax, dword ptr ss : [esp + 0x720]
			mov byte ptr ss : [esp + 0x310] , dl
			test byte ptr ds : [eax + 0x15] , 10
			je make_path
			mov edx, ebp
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_next_file
			mov eax, dword ptr ss : [esp + 0x720]
			mov edx, edi
			add eax, esi
			call fo::funcoffs::strcmp_
			test eax, eax
			je fs_next_file
			mov byte ptr ss : [esp + 0x310] , 1
			push 0
			mov ecx, dword ptr ss : [esp + 0x724]
			lea ebx, dword ptr ss : [esp + 0x620]
			lea edx, dword ptr ss : [esp + 0x730]
			lea eax, dword ptr ss : [esp + 0x210]
			add ecx, esi
			call fo::funcoffs::_makepath_
			lea eax, dword ptr ss : [esp + 0x20C]
			push eax
			//call dword ptr ss : [esp + 0x72C]
			call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
			add esp, 4
			test eax, eax
			je ret_close
		fs_next_file:
			lea eax, dword ptr ss : [esp + 0x71C]
			call fo::funcoffs::xsys_findnext_
			test eax, eax
			jne fs_compare_files
		ret_close:
			lea eax, dword ptr ss : [esp + 0x71C]
			call fo::funcoffs::xsys_findclose_
		ret_:
			add esp, 0x730
			pop ebp
			pop edi
			pop esi
			pop ecx
			ret
		}
	}


	static void* xenum_files(char* searchstring)
	{
		void* result = malloc(12);
		__asm {
			mov eax, searchstring
			mov ecx, result
			mov ebx, result
			mov ebp, result
			mov edx, 0x4E0278 // xlistenumfunc
			call _xenum_files
		}
		return result;
	}

	void Sandbox::init() { 
		//sfall::MakeCall(0x4DFF33, _xenum_files);
		//sfall::MakeJump(0x480A23, LoadScreenInitHook);
	}

}
