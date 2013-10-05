;----------------------------------------
; based upon a script of "Written by KiCHiK 2003-01-18 05:57:02"
;----------------------------------------
!verbose 3
!include "WinMessages.NSH"
!verbose 4
;----------------------------------------------------
!define NT_current_env 'HKCU "Environment"'
!define NT_all_env     'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
;====================================================
; IsNT - Returns 1 if the current system is NT, 0
;        otherwise.
;     Output: head of the stack
;====================================================
!macro IsNT UN
Function ${UN}IsNT
  Push $0
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 IsNT_yes
  ; we are not NT.
  Pop $0
  Push 0
  Return
 
  IsNT_yes:
    ; NT!!!
    Pop $0
    Push 1
FunctionEnd
!macroend
!insertmacro IsNT ""
!insertmacro IsNT "un."
;====================================================
; AddToPath - Adds the given dir to the search path.
;        Input - head of the stack
;        Note - Win9x systems requires reboot
;====================================================
Function AddToPath
   Exch $0
   Push $1
   Push $2
 
   Call IsNT
   Pop $1
   StrCmp $1 1 AddToPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" a
      FileSeek $1 0 END
      GetFullPathName /SHORT $0 $0
      FileWrite $1 "$\r$\nSET PATH=$0;%PATH%$\r$\n"
      FileClose $1
      Goto AddToPath_done
 
   AddToPath_NT:
      Push $4
      ; Try to set PATH for all.
      StrCpy $4 "all"
 
      AddToPath_NT_selection_done:
      StrCmp $4 "current" read_path_NT_current
         ReadRegStr $1 ${NT_all_env} "PATH"
         Goto read_path_NT_resume
      read_path_NT_current:
         ReadRegStr $1 ${NT_current_env} "PATH"
      read_path_NT_resume:
      StrCpy $2 $0
      StrCmp $1 "" AddToPath_NTdoIt
         StrCpy $2 "$0;$1"
      AddToPath_NTdoIt:
         StrCmp $4 "current" write_path_NT_current
            ClearErrors
            WriteRegExpandStr ${NT_all_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            ; Try again for the current user.
            StrCpy $4 "current"
            Goto AddToPath_NT_selection_done
         write_path_NT_current:
            ClearErrors
            WriteRegExpandStr ${NT_current_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            MessageBox MB_OK|MB_ICONINFORMATION "PATH could not be updated for all users or the current user."
            Goto write_path_NT_failed
         write_path_NT_resume:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
         DetailPrint "added path for user ($4), $0"
         write_path_NT_failed:
 
      Pop $4
   AddToPath_done:
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
 
;====================================================
; RemoveFromPath - Remove a given dir from the path
;     Input: head of the stack
;====================================================
Function un.RemoveFromPath
   Exch $0
   Push $1
   Push $2
   Push $3
   Push $4
   Push $5
 
   Call un.IsNT
   Pop $1
   StrCmp $1 1 unRemoveFromPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" r
      GetTempFileName $4
      FileOpen $2 $4 w
      GetFullPathName /SHORT $0 $0
      StrCpy $0 "SET PATH=%PATH%;$0"
      SetRebootFlag true
      Goto unRemoveFromPath_dosLoop
 
      unRemoveFromPath_dosLoop:
         FileRead $1 $3
         StrCmp $3 "$0$\r$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0" unRemoveFromPath_dosLoop
         StrCmp $3 "" unRemoveFromPath_dosLoopEnd
         FileWrite $2 $3
         Goto unRemoveFromPath_dosLoop
 
      unRemoveFromPath_dosLoopEnd:
         FileClose $2
         FileClose $1
         StrCpy $1 $WINDIR 2
         Delete "$1\autoexec.bat"
         CopyFiles /SILENT $4 "$1\autoexec.bat"
         Delete $4
         Goto unRemoveFromPath_done
 
   unRemoveFromPath_NT:
      StrLen $2 $0
      ; Try to unset PATH for all.
      StrCpy $4 "all"
      ReadRegStr $1 ${NT_all_env} "PATH"
 
   unRemoveFromPath_find:
      Push $1
      Push $0
      Call un.StrStr ; Find $0 in $1
      Pop $5 ; pos of our dir
      IntCmp $5 -1 unRemoveFromPath_next
         ; else, it is in path
         StrCpy $3 $1 $5 ; $3 now has the part of the path before our dir
         IntOp $2 $2 + $5 ; $2 now contains the pos after our dir in the path (';')
         IntOp $2 $2 + 1 ; $2 now containts the pos after our dir and the semicolon.
         StrLen $5 $1
         StrCpy $1 $1 $5 $2
         StrCpy $3 "$3$1"
 
         StrCmp $4 "current" un_write_path_NT_current
            WriteRegExpandStr ${NT_all_env} "PATH" $3
            Goto unRemoveFromPath_next
         un_write_path_NT_current:
            WriteRegExpandStr ${NT_current_env} "PATH" $3
      unRemoveFromPath_next:
         StrCmp $4 "current" unRemoveFromPath_done
	    ; Go round again for the current user.
            StrCpy $4 "current"
            ReadRegStr $1 ${NT_current_env} "PATH"
            Goto unRemoveFromPath_find
      unRemoveFromPath_done:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
   Pop $5
   Pop $4
   Pop $3
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall sutff
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
 
;====================================================
; StrStr - Finds a given string in another given string.
;               Returns -1 if not found and the pos if found.
;          Input: head of the stack - string to find
;                      second in the stack - string to find in
;          Output: head of the stack
;====================================================
Function un.StrStr
  Push $0
  Exch
  Pop $0 ; $0 now have the string to find
  Push $1
  Exch 2
  Pop $1 ; $1 now have the string to find in
  Exch
  Push $2
  Push $3
  Push $4
  Push $5
 
  StrCpy $2 -1
  StrLen $3 $0
  StrLen $4 $1
  IntOp $4 $4 - $3
 
  unStrStr_loop:
    IntOp $2 $2 + 1
    IntCmp $2 $4 0 0 unStrStrReturn_notFound
    StrCpy $5 $1 $3 $2
    StrCmp $5 $0 unStrStr_done unStrStr_loop
 
  unStrStrReturn_notFound:
    StrCpy $2 -1
 
  unStrStr_done:
    Pop $5
    Pop $4
    Pop $3
    Exch $2
    Exch 2
    Pop $0
    Pop $1
FunctionEnd
;====================================================
