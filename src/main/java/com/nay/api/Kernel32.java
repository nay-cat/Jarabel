package com.nay.api;

import com.sun.jna.*;
import com.sun.jna.platform.win32.WinBase;
import com.sun.jna.platform.win32.WinDef;
import com.sun.jna.platform.win32.WinNT.HANDLE;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.win32.StdCallLibrary;

public interface Kernel32 extends StdCallLibrary {
    Kernel32 INSTANCE = Native.load("kernel32", Kernel32.class);

    HANDLE CreateFileW(WString lpFileName, int dwDesiredAccess, int dwShareMode, Pointer lpSecurityAttributes,
                       int dwCreationDisposition, int dwFlagsAndAttributes, HANDLE hTemplateFile);

    boolean DeviceIoControl(HANDLE hDevice, int dwIoControlCode, Pointer lpInBuffer, int nInBufferSize,
                            Pointer lpOutBuffer, int nOutBufferSize, IntByReference lpBytesReturned, Pointer lpOverlapped);

    boolean CloseHandle(HANDLE hObject);

    int GetLastError();

    boolean FileTimeToSystemTime(WinBase.FILETIME ft, WinBase.SYSTEMTIME st);

    boolean ReadFile(HANDLE hVolume, byte[] tempBuffer, int mftRecordSize, WinDef.DWORDByReference bytesRead, Object o);

}
