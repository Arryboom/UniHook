#ifndef INJECTOR_H
#define INJECTOR_H

#include <windows.h>
#include <QString>
#include <QDebug>

BOOL InjectDLL( HANDLE hProcess, QString file )
{
    wchar_t wszDllFile[1024] = {0};
    file.toWCharArray( wszDllFile );

    if ( !hProcess )
    {
        qDebug( ) << "[!] Failed to call OpenProcess, GetLastError( ) = %d\n" << GetLastError( );
        return FALSE;
    }

    LPVOID lpRemoteAddress = VirtualAllocEx( hProcess, NULL, wcslen( wszDllFile ) * sizeof( wchar_t ), MEM_COMMIT, PAGE_READWRITE );

    if( !lpRemoteAddress )
    {
        qDebug( ) << "[!] Failed to call VirtualAllocEx Handle: %I64X, GetLastError( ) = %d\n" << GetLastError( );
        return FALSE;
    }

    SIZE_T bytes;

    if( !WriteProcessMemory( hProcess, lpRemoteAddress, wszDllFile, wcslen( wszDllFile ) * sizeof( wchar_t ), &bytes ) )
    {
        qDebug( ) << "[!] Failed to call WriteProcessMemory( ), GetLastError( ) = %d\n" << GetLastError( );
        return FALSE;
    }

    FARPROC lpLoadLibrary = GetProcAddress( GetModuleHandle( L"KERNEL32.DLL" ), "LoadLibraryW" );

    if ( !lpLoadLibrary )
    {
        qDebug( ) << "[!] Failed to get address of LoadLibrary\n";
        return FALSE;
    }

    HANDLE hThread =
        CreateRemoteThread( hProcess, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>( lpLoadLibrary ), lpRemoteAddress, NULL, NULL );

    if( !hThread )
    {
        qDebug( ) << "[!] Failed to call CreateRemoteThread, GetLastError( ) = %d\n" << GetLastError( );
        return FALSE;
    }

    WaitForSingleObject( hThread, INFINITE );

    if( !VirtualFreeEx( hProcess, lpRemoteAddress, 0, MEM_RELEASE ) )
    {
        qDebug( ) << "[!] Failed to call VirtualFreeEx, GetLastError( ) = %d\n" << GetLastError( );
        return FALSE;
    }

    CloseHandle( hThread );

    return TRUE;
}

#endif // INJECTOR


