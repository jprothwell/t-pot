//=============================================================================
// T-PoT - Total Commander file system plug-in for iPod and iPhone devices
//-----------------------------------------------------------------------------
// File:			iPoTApi.cpp
// Purpose:			iPod/iPhone devices USB file interface API class.
// Limitations:		Based on undocumented features of iTunes.
//					Tested with iTunes 7.4 and iPod firmware 1.1.1 / 1.1.2.
// Platform:		Win32
//-----------------------------------------------------------------------------
// Copyright (c) 2007-2009, Scythal
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of the software nor the names of its contributors may be 
//   used to endorse or promote products derived from this software without 
//   specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY ITS AUTHOR ``AS IS'' AND ANY EXPRESS OR 
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
// NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//=============================================================================

#pragma warning(disable:4996)

#include "stdafx.h"

#include <atlstr.h>			// To use CString (instead of afxwin.h)
#include <sys/types.h>
#include <sys/stat.h>

#include "PngConv.h"
#include "iPoTApi.h"

// ----------------------------------------------------------------------------
// Static class data
// ----------------------------------------------------------------------------

volatile t_iPodState CiPoTApi::m_iPodState = IPOD_STATE_NO_DLL;
volatile t_AMDevice *CiPoTApi::m_iPodDev = NULL;
volatile unsigned int CiPoTApi::m_GlobalConnectionID = 1;
char *CiPoTApi::m_piTunesMobileDevicePath = NULL;
char *CiPoTApi::m_pCoreFoundationPath = NULL;
HINSTANCE CiPoTApi::m_iTunesDll = NULL;
HINSTANCE CiPoTApi::m_CoreFoundationDll = NULL;
bool CiPoTApi::bCanTranslatePLIST = false;

// ----------------------------------------------------------------------------
// Class constructor & destructor
// ----------------------------------------------------------------------------

CiPoTApi::CiPoTApi()
{
	m_ProgressCallBack = NULL;
	ResetConnection();
}

CiPoTApi::~CiPoTApi()
{
	DetachDLL();
}

// ----------------------------------------------------------------------------
// Static class methods
// ----------------------------------------------------------------------------

tf_AMDeviceNotificationSubscribe	        CiPoTApi::AMDeviceNotificationSubscribe;
tf_AMDeviceConnect					        CiPoTApi::AMDeviceConnect;
tf_AMDeviceDisconnect				        CiPoTApi::AMDeviceDisconnect;
tf_AMDeviceIsPaired					        CiPoTApi::AMDeviceIsPaired;
tf_AMDeviceValidatePairing			        CiPoTApi::AMDeviceValidatePairing;
tf_AMDeviceStartSession				        CiPoTApi::AMDeviceStartSession;
tf_AMDeviceStartService				        CiPoTApi::AMDeviceStartService;
tf_AMDeviceStopSession				        CiPoTApi::AMDeviceStopSession;
tf_AFCConnectionOpen				        CiPoTApi::AFCConnectionOpen;
tf_AFCDeviceInfoOpen				        CiPoTApi::AFCDeviceInfoOpen;
tf_AFCDirectoryOpen					        CiPoTApi::AFCDirectoryOpen;
tf_AFCDirectoryRead					        CiPoTApi::AFCDirectoryRead;
tf_AFCDirectoryClose				        CiPoTApi::AFCDirectoryClose;
tf_AFCFileInfoOpen					        CiPoTApi::AFCFileInfoOpen;
tf_AFCKeyValueRead					        CiPoTApi::AFCKeyValueRead;
tf_AFCKeyValueClose					        CiPoTApi::AFCKeyValueClose;
tf_AFCFileRefOpen					        CiPoTApi::AFCFileRefOpen;
tf_AFCFileRefClose					        CiPoTApi::AFCFileRefClose;
tf_AFCFileRefRead					        CiPoTApi::AFCFileRefRead;
tf_AFCFileRefWrite					        CiPoTApi::AFCFileRefWrite;
tf_AFCRemovePath					        CiPoTApi::AFCRemovePath;
tf_AFCDirectoryCreate				        CiPoTApi::AFCDirectoryCreate;
tf_AFCRenamePath					        CiPoTApi::AFCRenamePath;

tf_CFWriteStreamCreateWithFile				CiPoTApi::CFWriteStreamCreateWithFile;
tf_CFReadStreamCreateWithFile				CiPoTApi::CFReadStreamCreateWithFile;
tf_CFStringCreateWithCString				CiPoTApi::CFStringCreateWithCString;
tf_CFURLCreateWithFileSystemPath			CiPoTApi::CFURLCreateWithFileSystemPath;
tf_CFReadStreamOpen							CiPoTApi::CFReadStreamOpen;
tf_CFWriteStreamOpen						CiPoTApi::CFWriteStreamOpen;
tf_CFPropertyListCreateFromStream			CiPoTApi::CFPropertyListCreateFromStream;
tf_CFReadStreamClose						CiPoTApi::CFReadStreamClose;
tf_CFPropertyListIsValid					CiPoTApi::CFPropertyListIsValid;
tf_CFPropertyListWriteToStream				CiPoTApi::CFPropertyListWriteToStream;
tf_CFWriteStreamClose						CiPoTApi::CFWriteStreamClose;
tf_CFRelease								CiPoTApi::CFRelease;
tf_CFURLCreateDataAndPropertiesFromResource	CiPoTApi::CFURLCreateDataAndPropertiesFromResource;
tf_CFPropertyListCreateFromXMLData          CiPoTApi::CFPropertyListCreateFromXMLData;
tf_CFPropertyListCreateXMLData              CiPoTApi::CFPropertyListCreateXMLData;
tf_CFURLWriteDataAndPropertiesToResource    CiPoTApi::CFURLWriteDataAndPropertiesToResource;

// Attaches the API: loads the DLL
// Returns:
//		IPOD_ERR_OK				when successful
//		IPOD_ERR_DLL_NOT_FOUND	when the iTunesMobileDevice.dll could not be loaded
//
t_iPodError CiPoTApi::AttachDLL()
{
	HKEY hSetting = NULL;
	DWORD length = 0;
	CString path;
	int pos;
	t_MachError ret;
	t_AMDeviceNotification *notif; 

	if (m_iPodState == IPOD_STATE_NO_DLL) {
		if (m_piTunesMobileDevicePath == NULL) {
			// Adds iTunesMobileDevice.dll folder to the path, from the registry:
			if (::RegCreateKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Apple Inc.\\Apple Mobile Device Support\\Shared"), &hSetting) != ERROR_SUCCESS)
				return IPOD_ERR_DLL_NOT_FOUND;
			if (::RegQueryValueEx(hSetting, _T("iTunesMobileDeviceDLL"), NULL, NULL, NULL, &length) != ERROR_SUCCESS)
				return IPOD_ERR_DLL_NOT_FOUND;
			m_piTunesMobileDevicePath = new char[length+1];
			::RegQueryValueEx(hSetting, _T("iTunesMobileDeviceDLL"), NULL, NULL, (LPBYTE)m_piTunesMobileDevicePath, &length);

			// Adds the folder to the current system path:	
			path.GetEnvironmentVariable("PATH");
			path = (path + ";") + m_piTunesMobileDevicePath;
			pos = path.ReverseFind('\\');
			if (pos >= 0)
				path = path.Left(pos);
			SetEnvironmentVariable("PATH", path);
		}

		// Loads the iTunesMobileDevice DLL routines
		m_iTunesDll = LoadLibrary(m_piTunesMobileDevicePath);
		//m_iTunesDll = LoadLibrary("C:\\Program Files\\Common Files\\Apple\\Mobile Device Support\\bin\\MobileDevice.dll");
		if (m_iTunesDll) {
			AMDeviceNotificationSubscribe = (tf_AMDeviceNotificationSubscribe)GetProcAddress(m_iTunesDll, "AMDeviceNotificationSubscribe");
			AMDeviceConnect = (tf_AMDeviceConnect)GetProcAddress(m_iTunesDll, "AMDeviceConnect");
			AMDeviceDisconnect = (tf_AMDeviceDisconnect)GetProcAddress(m_iTunesDll, "AMDeviceDisconnect");
			AMDeviceIsPaired = (tf_AMDeviceIsPaired)GetProcAddress(m_iTunesDll, "AMDeviceIsPaired");
			AMDeviceValidatePairing = (tf_AMDeviceValidatePairing)GetProcAddress(m_iTunesDll, "AMDeviceValidatePairing");
			AMDeviceStartSession = (tf_AMDeviceStartSession)GetProcAddress(m_iTunesDll, "AMDeviceStartSession");
			AMDeviceStartService = (tf_AMDeviceStartService)GetProcAddress(m_iTunesDll, "AMDeviceStartService");
			AMDeviceStopSession = (tf_AMDeviceStopSession)GetProcAddress(m_iTunesDll, "AMDeviceStopSession");
			AFCConnectionOpen = (tf_AFCConnectionOpen)GetProcAddress(m_iTunesDll, "AFCConnectionOpen");
			AFCDeviceInfoOpen = (tf_AFCDeviceInfoOpen)GetProcAddress(m_iTunesDll, "AFCDeviceInfoOpen");
			AFCDirectoryOpen = (tf_AFCDirectoryOpen)GetProcAddress(m_iTunesDll, "AFCDirectoryOpen");
			AFCDirectoryRead = (tf_AFCDirectoryRead)GetProcAddress(m_iTunesDll, "AFCDirectoryRead");
			AFCDirectoryClose = (tf_AFCDirectoryClose)GetProcAddress(m_iTunesDll, "AFCDirectoryClose");
			AFCFileInfoOpen = (tf_AFCFileInfoOpen)GetProcAddress(m_iTunesDll, "AFCFileInfoOpen");
			AFCKeyValueRead = (tf_AFCKeyValueRead)GetProcAddress(m_iTunesDll, "AFCKeyValueRead");
			AFCKeyValueClose = (tf_AFCKeyValueClose)GetProcAddress(m_iTunesDll, "AFCKeyValueClose");
			AFCFileRefOpen = (tf_AFCFileRefOpen)GetProcAddress(m_iTunesDll, "AFCFileRefOpen");
			AFCFileRefClose = (tf_AFCFileRefClose)GetProcAddress(m_iTunesDll, "AFCFileRefClose");
			AFCFileRefRead = (tf_AFCFileRefRead)GetProcAddress(m_iTunesDll, "AFCFileRefRead");
			AFCFileRefWrite = (tf_AFCFileRefWrite)GetProcAddress(m_iTunesDll, "AFCFileRefWrite");
			AFCRemovePath = (tf_AFCRemovePath)GetProcAddress(m_iTunesDll, "AFCRemovePath");
			AFCDirectoryCreate = (tf_AFCDirectoryCreate)GetProcAddress(m_iTunesDll, "AFCDirectoryCreate");
			AFCRenamePath = (tf_AFCRenamePath)GetProcAddress(m_iTunesDll, "AFCRenamePath");
		} else
			return IPOD_ERR_DLL_NOT_FOUND;
		
		// Loads the CoreFoundation DLL routines
		m_CoreFoundationDll = LoadLibrary("CoreFoundation.dll");
		if (m_CoreFoundationDll) {
           CFWriteStreamCreateWithFile = (tf_CFWriteStreamCreateWithFile)GetProcAddress(m_CoreFoundationDll, "CFWriteStreamCreateWithFile");;
           CFReadStreamCreateWithFile = (tf_CFReadStreamCreateWithFile)GetProcAddress(m_CoreFoundationDll, "CFReadStreamCreateWithFile");;
           CFStringCreateWithCString = (tf_CFStringCreateWithCString)GetProcAddress(m_CoreFoundationDll, "CFStringCreateWithCString");;
           CFURLCreateWithFileSystemPath = (tf_CFURLCreateWithFileSystemPath)GetProcAddress(m_CoreFoundationDll, "CFURLCreateWithFileSystemPath");;
           CFReadStreamOpen = (tf_CFReadStreamOpen)GetProcAddress(m_CoreFoundationDll, "CFReadStreamOpen");;
           CFWriteStreamOpen = (tf_CFWriteStreamOpen)GetProcAddress(m_CoreFoundationDll, "CFWriteStreamOpen");;
           CFPropertyListCreateFromStream = (tf_CFPropertyListCreateFromStream)GetProcAddress(m_CoreFoundationDll, "CFPropertyListCreateFromStream");;
           CFReadStreamClose = (tf_CFReadStreamClose)GetProcAddress(m_CoreFoundationDll, "CFReadStreamClose");;
           CFPropertyListIsValid = (tf_CFPropertyListIsValid)GetProcAddress(m_CoreFoundationDll, "CFPropertyListIsValid");;
           CFPropertyListWriteToStream = (tf_CFPropertyListWriteToStream)GetProcAddress(m_CoreFoundationDll, "CFPropertyListWriteToStream");;
           CFWriteStreamClose = (tf_CFWriteStreamClose)GetProcAddress(m_CoreFoundationDll, "CFWriteStreamClose");;
           CFRelease = (tf_CFRelease)GetProcAddress(m_CoreFoundationDll, "CFRelease");;
           CFURLCreateDataAndPropertiesFromResource = (tf_CFURLCreateDataAndPropertiesFromResource)GetProcAddress(m_CoreFoundationDll, "CFURLCreateDataAndPropertiesFromResource");;
           CFPropertyListCreateFromXMLData = (tf_CFPropertyListCreateFromXMLData)GetProcAddress(m_CoreFoundationDll, "CFPropertyListCreateFromXMLData");;
           CFPropertyListCreateXMLData = (tf_CFPropertyListCreateXMLData)GetProcAddress(m_CoreFoundationDll, "CFPropertyListCreateXMLData");;
           CFURLWriteDataAndPropertiesToResource = (tf_CFURLWriteDataAndPropertiesToResource)GetProcAddress(m_CoreFoundationDll, "CFURLWriteDataAndPropertiesToResource");;
            bCanTranslatePLIST = true;
		} else
			bCanTranslatePLIST = false;
		
		m_iPodState = IPOD_STATE_UNCONNECTED;
	}
	
	// Registers the static notification call-back method
	ret = AMDeviceNotificationSubscribe(&CiPoTApi::NotificationHandler, 0, 0, 0, &notif);
	return (ret == MDERR_OK) ? IPOD_ERR_OK : IPOD_IPOD_NOT_FOUND;
}

// Detachs the API: unloads the DLL and clears private data.
// Returns:
//		IPOD_ERR_OK				when successful
//
t_iPodError CiPoTApi::DetachDLL()
{
	delete m_piTunesMobileDevicePath;
	m_piTunesMobileDevicePath = NULL;
	if (m_iPodState != IPOD_STATE_NO_DLL) {
		FreeLibrary(m_iTunesDll);
		m_iPodState = IPOD_STATE_NO_DLL;
	}
	return IPOD_ERR_OK;
}

// Callback notification routine called by iTunesMobileDevice.dll.
//
void CiPoTApi::NotificationHandler(t_AMDeviceNotificationInfo *info)
{
	unsigned int msg = info->msg;
	
	switch (msg) {
		case ADNCI_MSG_CONNECTED:
			if (m_iPodState == IPOD_STATE_UNCONNECTED) {
				if (AMDeviceConnect(info->dev))
					// Restore mode not supported yet in this API
					return;
				if (!AMDeviceIsPaired(info->dev))
					return;
				if (AMDeviceValidatePairing(info->dev))
					return;
				if (AMDeviceStartSession(info->dev))
					return;
				m_iPodDev = info->dev;
				m_iPodState = IPOD_STATE_CONNECTED;
				m_GlobalConnectionID++;
			} else {
				// Was in another state before, something bad must have happened
			}
			break;
		case ADNCI_MSG_DISCONNECTED:
			if (m_iPodState == IPOD_STATE_CONNECTED) {
				m_iPodState = IPOD_STATE_UNCONNECTED;
				m_iPodDev = NULL;
			}
			break;
		default:
			break;
	}
}

// ----------------------------------------------------------------------------
// Non-static class methods
// ----------------------------------------------------------------------------

// Resets the connection parameters.
//
void CiPoTApi::ResetConnection()
{
	m_pDev = NULL;
	m_iPodAFCName = AMSVC_AFC2;
	m_iPodAFC = NULL;
	m_iPodConnection = NULL;
	m_ApiState = API_STATE_UNAVAILABLE;
	m_ConnectionID = 0;
}

// Sets the progress callback function (NULL to remove it).
//
void CiPoTApi::SetProgressCallBack(tf_Progress fn)
{
	m_ProgressCallBack = fn;
}

// Checks that the connection is still active and that it hasn't been
// renewed since the last access (otherwise the parameters will not be
// valid anymore: m_iPodConnection, m_iPodAFC).
//
void CiPoTApi::CheckConnection()
{
	if (m_iPodState != IPOD_STATE_CONNECTED || m_ConnectionID != m_GlobalConnectionID)
		// iPod was disconnected since last time
		ResetConnection();
}

// Checks the connection state and returns the up-to-date API state.
// Doesn't try to establish a new connection.
//
t_ApiState CiPoTApi::GetApiState()
{
	CheckConnection();
	return m_ApiState;
}

// Opens a session on the iPod. This should be called before starting any
// operation to ensure the iPod hasn't been disconnected since the last time,
// or simply to initialize the first connection if it hasn't been done before.
//
// Returns:
//		IPOD_ERR_OK				when successful
//		IPOD_IPOD_NOT_FOUND		if no iPod is connected to any USB port
//		IPOD_COULD_NOT_CONNECT	if the connection could not be established
//
t_iPodError CiPoTApi::OpenSession()
{
	t_MachError ret;
	int timeout;

	CheckConnection();
	if (m_ApiState == API_STATE_AVAILABLE)
			// Session is already open
			return IPOD_ERR_OK;

	// Checks if iPod connected
	for (timeout = 0; (timeout < 200) && (m_iPodState != IPOD_STATE_CONNECTED); timeout++)
		Sleep(10);
	if (m_iPodState != IPOD_STATE_CONNECTED)
		return IPOD_IPOD_NOT_FOUND;

	// Opens a session for this instance
	m_pDev = (t_AMDevice *)m_iPodDev;
	if (AMDeviceStartService(m_pDev, m_iPodAFCName, &m_iPodAFC, NULL)) {
		// Not jailbroken, tries to connect to the standard name
		m_iPodAFCName = AMSVC_AFC;
		ret = AMDeviceStartService(m_pDev, m_iPodAFCName, &m_iPodAFC, NULL);
		if (ret)
			return IPOD_COULD_NOT_CONNECT;
	}
	if (AFCConnectionOpen(m_iPodAFC, 0, &m_iPodConnection))
		return IPOD_COULD_NOT_CONNECT;
	m_ApiState = API_STATE_AVAILABLE;
	m_ConnectionID = m_GlobalConnectionID;
	return IPOD_ERR_OK;
}

// Closes the session.
//
// Returns:
//		IPOD_ERR_OK				when successful
//
t_iPodError CiPoTApi::CloseSession()
{
	if (m_ApiState == API_STATE_AVAILABLE) {
		/* Doesn't work (crash), so basically we never disconnect, it
		   doesn't seem to lock any resources anyway:
		   
			AMDeviceStopSession((t_AMDevice *)m_iPodDev);
			AMDeviceDisconnect((t_AMDevice *)m_iPodDev);
		*/
	}
	m_ApiState = API_STATE_UNAVAILABLE;
	return IPOD_ERR_OK;
}

// ----------------------------------------------------------------------------

// Gets a temporary filename (in the %TEMP% directory), for example to create
// a temporary local copy of iPod files.
//
void CiPoTApi::GetTempFilename(CString &tmpFilename)
{
    char lpPathBuffer[MAX_PATH];
    char szTempName[MAX_PATH];

    GetTempPath(MAX_PATH, lpPathBuffer);
    GetTempFileName(lpPathBuffer, "TCMD", 0, szTempName);
	tmpFilename = szTempName;
}

// Puts the information on a particular file in the pInfo structure.
//
// Returns:
//		MDERR_OK				when successful
//		MDERR_AFC_NOT_FOUND		if the file couldn't be found (or is a broken link)
//
t_MachError CiPoTApi::GetFileInfo(t_iPodFileInfo *pInfo, char *remotePath)
{
	t_MachError ret;
	struct afc_dictionary *pDict;
	char *pKey, *pVal;

	pInfo->findData.dwFileAttributes = 0;
	memset(&pInfo->findData.ftCreationTime, 0, sizeof(pInfo->findData.ftCreationTime));
	memset(&pInfo->findData.ftLastAccessTime, 0, sizeof(pInfo->findData.ftLastAccessTime));
	memset(&pInfo->findData.ftLastWriteTime, 0, sizeof(pInfo->findData.ftLastWriteTime));
	pInfo->findData.nFileSizeHigh = 0;
	pInfo->findData.nFileSizeLow = 0;
	if ((ret = AFCFileInfoOpen(m_iPodConnection, remotePath, &pDict)) != MDERR_OK)
		return ret;
	AFCKeyValueRead(pDict, &pKey, &pVal);
	while(pKey || pVal) {
		if (!strcmp(pKey, "st_ifmt")) {
			if (!strcmp(pVal, "S_IFIFO")) {
				// named pipe (fifo), how are you supposed to translate that in Windows?
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
			} else if (!strcmp(pVal, "S_IFCHR")) {
				// character special
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
			} else if (!strcmp(pVal, "S_IFDIR")) {
				// directory
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			} else if (!strcmp(pVal, "S_IFBLK")) {
				// block special
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
			} else if (!strcmp(pVal, "S_IFREG")) {
				// regular
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
			} else if (!strcmp(pVal, "S_IFLNK")) {
				// symbolic link
			} else if (!strcmp(pVal, "S_IFSOCK")) {
				// socket
				pInfo->findData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
			}
 		} else if (!strcmp(pKey, "st_size")) {
 			pInfo->findData.nFileSizeLow = pInfo->iPodSize = strtol(pVal, NULL, 10);
 		} else if (!strcmp(pKey, "st_blocks")) {
 			pInfo->iPodBlocks = atoi(pVal);
 		}
		AFCKeyValueRead(pDict, &pKey, &pVal);
	}
	if (pInfo->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		pInfo->findData.nFileSizeLow = 0;
	}
	AFCKeyValueClose(pDict);
	return MDERR_OK;
}

// Puts the information on a particular file in the pInfo structure. The name of the
// file is the concatenation of:
//		pInfo->remotePath (its directory) and
//		pInfo->findData.cFileName (its name)
// This overload makes for easier use with FindFirst/FindNext methods.
//
// Returns:
//		MDERR_OK				when successful
//		MDERR_AFC_NOT_FOUND		if the file couldn't be found (or is a broken link)
//
t_MachError CiPoTApi::GetFileInfo(t_iPodFileInfo *pInfo)
{
	CString filename(pInfo->remotePath);
	
	if (filename.Right(1).Compare("/"))
		filename += "/";
	filename += pInfo->findData.cFileName;
	
	return GetFileInfo(pInfo, filename.GetBuffer());
}

// Finds the first file of a given directory. Must be used in conjunction with 
// the FindNext and FindClose methods. FindClose needn't be called if the call
// to FindFirst returned false.
//
// Returns:
//		true	if a file was found
//		false	if no file was found, or if there was an error
//
// Example:
//
//		CiPoTApi iPodApi;
//		t_iPodFileInfo info;
//		bool bFile;
//		t_iPodError status = iPodApi.OpenSession();
//
//		if (status == IPOD_ERR_OK) {
//			bFile = iPodApi.FindFirst(remotePath, &info);
//			while (bFile) {
//				printf("%s\n", info.findData.cFileName);
//				bFile = iPodApi.FindNext(&info);
//			}
//			iPodApi.FindClose(&info);
//
bool CiPoTApi::FindFirst(char *remotePath, t_iPodFileInfo *pInfo)
{
	t_AFCDirectory *pHandle;
	char *pEntry;
	afc_error_t ret;

	pInfo->pHandle = NULL;
	pInfo->remotePath = NULL;
	ret = AFCDirectoryOpen(m_iPodConnection, remotePath, &pHandle);
	if (ret)
		return false;
	ret = AFCDirectoryRead(m_iPodConnection, pHandle, &pEntry);
	if (ret || pEntry == NULL) {
		// Broken link?
		AFCDirectoryClose(m_iPodConnection, pHandle);
		return false;
	}
	pInfo->remotePath = strdup (remotePath);
	strncpy(pInfo->findData.cFileName, pEntry, MAX_PATH);
	pInfo->pHandle = pHandle;
	GetFileInfo(pInfo);
	return true;
}

// Finds the next file in the directory. Used in conjunction with FindFirst 
// and FindClose.
//
// Returns:
//		true	if a file was found
//		false	if no more file was found, or if there was an error
//
bool CiPoTApi::FindNext(t_iPodFileInfo *pInfo)
{
	char *pEntry;
	afc_error_t ret;

	ret = AFCDirectoryRead(m_iPodConnection, pInfo->pHandle, &pEntry);
	if (ret || pEntry == NULL)
		return false;
	strncpy(pInfo->findData.cFileName, pEntry, MAX_PATH);
	GetFileInfo(pInfo);
	return true;
}

// Closes the directory search request. Used in conjunction with FindFirst
// and FindNext. Must be called if the first call to FindFirst returned
// a true value.
//
void CiPoTApi::FindClose(t_iPodFileInfo *pInfo)
{
	if (pInfo->remotePath)
		free(pInfo->remotePath);
	if (pInfo->pHandle)
		AFCDirectoryClose(m_iPodConnection, pInfo->pHandle);
}

// Checks if a file (or directory, device, ...) exists.
//
// Returns:
//		true	if the file exists
//		false	if the file doesn't exist
//
bool CiPoTApi::FileExists(char *remotePath)
{
	t_iPodFileInfo info;
	
	return (GetFileInfo(&info, remotePath) == MDERR_OK);
}

// Reads a file from the iPod and stores it on the PC filesystem.
// 
// Returns:
//		MDERR_OK				when successful
//		MDERR_AFC_NOT_FOUND		if the file couldn't be found (or is a broken link)
//		MDERR_AFC_ACCESS_DENIED	if access denied (e.g. directory, dir. link)
//		-1						if the file could not be entirely read
//
// TODO: cleaner return error codes
//
int CiPoTApi::FileRead(char *remotePath, char *localPath)
{
	t_iPodFileInfo info;
	t_MachError ret;
	unsigned char *buffer;
	FILE *fLocal;
	unsigned long total = 0;
	unsigned int len;
	afc_file_ref handle;

	ret = GetFileInfo(&info, remotePath);
	if (ret != MDERR_OK)
		return ret;
	ret = AFCFileRefOpen(m_iPodConnection, remotePath, AFC_FILEMODE_READ, 0, &handle);
	if (ret != MDERR_OK)
		return ret;
	buffer = new unsigned char[FILEREAD_BUFFER_SIZE];
	if (buffer) {
		fLocal = fopen(localPath, "wb");
		while (total < info.findData.nFileSizeLow) {
			len = (unsigned int)min(info.findData.nFileSizeLow - total, (unsigned long)FILEREAD_BUFFER_SIZE);
			ret = AFCFileRefRead(m_iPodConnection, handle, buffer, &len);
			if (ret != MDERR_OK || !len)
				break;
			if (fwrite(buffer, 1, len, fLocal) != len)
				break;
			total += len;
			if (m_ProgressCallBack(remotePath, localPath, (int)(100.0*total/info.findData.nFileSizeLow)))
				break;
		}
		delete[] buffer;
		fclose(fLocal);
	}
	AFCFileRefClose(m_iPodConnection, handle);
	return (total == info.findData.nFileSizeLow) ? MDERR_OK : -1;
}

// Writes a file from the PC filesystem and stores it on the iPod.
// 
// Returns:
//		MDERR_OK				when successful
//		MDERR_AFC_NOT_FOUND		if the file couldn't be found (or is a broken link)
//		MDERR_AFC_ACCESS_DENIED	if access denied (e.g. directory, dir. link)
//		-1						if the file could not be entirely  written
//
// TODO: cleaner return error codes
//
int CiPoTApi::FileWrite(char *remotePath, char *localPath)
{
	struct _stat info;
	t_MachError ret;
	unsigned char *buffer;
	FILE *fLocal;
	unsigned long len, total = 0;
	afc_file_ref handle;
	
	if (_stat(localPath, &info))
		return MDERR_AFC_NOT_FOUND;
	ret = AFCFileRefOpen(m_iPodConnection, remotePath, AFC_FILEMODE_WRITE, 0, &handle);	
	if (ret != MDERR_OK)
		return ret;
	buffer = new unsigned char[FILEREAD_BUFFER_SIZE];
	if (buffer) {
		fLocal = fopen(localPath, "rb");
		while (total < (unsigned long)info.st_size) {
			len = min(info.st_size - total, FILEREAD_BUFFER_SIZE);
			len = fread(buffer, 1, len, fLocal);
			if (!len)
				break;
			ret = AFCFileRefWrite(m_iPodConnection, handle, buffer, len);
			if (ret != MDERR_OK)
				break;
			total += len;
			if (m_ProgressCallBack(remotePath, localPath, (int)(100.0*total/info.st_size)))
				break;
		}
		delete[] buffer;
		fclose(fLocal);
	}
	AFCFileRefClose(m_iPodConnection, handle);
	return (total == info.st_size) ? MDERR_OK : -1;
}

// Moves a file or directory on the iPod.
//
// Returns:
//		MDERR_OK				when successful
//		??
//	
t_MachError CiPoTApi::Move(char *oldPath, char *newPath)
{
	return AFCRenamePath(m_iPodConnection, oldPath, newPath);
}

// Removes a file or a directory on the iPod.
//
// Returns:
//		MDERR_OK				when successful
//		??
//	
t_MachError CiPoTApi::Remove(char *remotePath)
{
	return AFCRemovePath(m_iPodConnection, remotePath);
}

// Makes a new directory on the iPod.
//
// Returns:
//		MDERR_OK				when successful
//		??
//	
t_MachError CiPoTApi::MakeDir(char *remotePath)
{
	return AFCDirectoryCreate(m_iPodConnection, remotePath);
}

// Translates an iPod PNG image file into a normal PNG file.
//
// Returns:
//		true	on success
//		false	on failure
//
bool CiPoTApi::TranslatePNG(char *originalFile, char *newFile)
{
	CPngConv converter;
	return converter.Convert(originalFile, newFile) == 0;
}

// Translates an iPod binary Property-list into a text property list.
//
// Returns:
//		true	on success
//		false	on failure
//
bool CiPoTApi::TranslatePLIST(char *originalFile, char *newFile)
{
	CFPropertyListRef propertyList;
	CFURLRef fileURL;
	CFStringRef inputFile, errorString;
	CFDataRef resourceData;
	Boolean status;
	SInt32 errorCode;
	bool ok = false;

	// Create a URL that specifies the file we will create to hold the XML data.
	inputFile = CFStringCreateWithCString(kCFAllocatorDefault, originalFile, kStrEncoding);
	fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inputFile, kPathStyle, false);
	if (!fileURL)
		return false;

	// Read the XML file.
	status = CFURLCreateDataAndPropertiesFromResource(
		kCFAllocatorDefault, fileURL, &resourceData, NULL, NULL, &errorCode);

	if (resourceData) {
		// Reconstitute the dictionary using the XML data.
		propertyList = CFPropertyListCreateFromXMLData(
			kCFAllocatorDefault, resourceData,kCFPropertyListImmutable, &errorString);
	
		if (propertyList != NULL) {
			CFStringRef outputFile;
			CFDataRef xmlData;
			Boolean status;
			SInt32 errorCode;
			
			CFRelease(fileURL);
			outputFile = CFStringCreateWithCString(kCFAllocatorDefault, newFile, kStrEncoding);
			fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, outputFile, kPathStyle, false);

			// Convert the property list into XML data.
			xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, propertyList);
			// Write the XML data to the file.
			status = CFURLWriteDataAndPropertiesToResource(fileURL, xmlData, NULL, &errorCode);
			ok = status ? true : false;
			CFRelease(xmlData);
			CFRelease(outputFile);
			CFRelease(propertyList);
		}
	}
		
	CFRelease(inputFile);
	CFRelease(fileURL);
	return ok;
}

// ----------------------------------------------------------------------------
