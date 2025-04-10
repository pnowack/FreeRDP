/**
 * WinPR: Windows Portable Runtime
 * File Functions
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2014 Hewlett-Packard Development Company, L.P.
 * Copyright 2015 Thincast Technologies GmbH
 * Copyright 2015 bernhard.miklautz@thincast.com
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <winpr/config.h>

#include <winpr/crt.h>
#include <winpr/path.h>
#include <winpr/file.h>

#ifdef WINPR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../log.h"
#define TAG WINPR_TAG("file")

#ifndef _WIN32

#ifdef ANDROID
#include <sys/vfs.h>
#else
#include <sys/statvfs.h>
#endif

#include "../handle/handle.h"

#include "../pipe/pipe.h"
#include "namedPipeClient.h"

static BOOL NamedPipeClientIsHandled(HANDLE handle)
{
	return WINPR_HANDLE_IS_HANDLED(handle, HANDLE_TYPE_NAMED_PIPE, TRUE);
}

static BOOL NamedPipeClientCloseHandle(HANDLE handle)
{
	WINPR_NAMED_PIPE* pNamedPipe = (WINPR_NAMED_PIPE*)handle;

	if (!NamedPipeClientIsHandled(handle))
		return FALSE;

	if (pNamedPipe->clientfd != -1)
	{
		// WLOG_DBG(TAG, "closing clientfd %d", pNamedPipe->clientfd);
		close(pNamedPipe->clientfd);
	}

	if (pNamedPipe->serverfd != -1)
	{
		// WLOG_DBG(TAG, "closing serverfd %d", pNamedPipe->serverfd);
		close(pNamedPipe->serverfd);
	}

	if (pNamedPipe->pfnUnrefNamedPipe)
		pNamedPipe->pfnUnrefNamedPipe(pNamedPipe);

	free(pNamedPipe->lpFileName);
	free(pNamedPipe->lpFilePath);
	free(pNamedPipe->name);
	free(pNamedPipe);
	return TRUE;
}

static int NamedPipeClientGetFd(HANDLE handle)
{
	WINPR_NAMED_PIPE* file = (WINPR_NAMED_PIPE*)handle;

	if (!NamedPipeClientIsHandled(handle))
		return -1;

	if (file->ServerMode)
		return file->serverfd;
	else
		return file->clientfd;
}

static HANDLE_OPS ops = {
	NamedPipeClientIsHandled,
	NamedPipeClientCloseHandle,
	NamedPipeClientGetFd,
	NULL, /* CleanupHandle */
	NamedPipeRead,
	NULL, /* FileReadEx */
	NULL, /* FileReadScatter */
	NamedPipeWrite,
	NULL, /* FileWriteEx */
	NULL, /* FileWriteGather */
	NULL, /* FileGetFileSize */
	NULL, /*  FlushFileBuffers */
	NULL, /* FileSetEndOfFile */
	NULL, /* FileSetFilePointer */
	NULL, /* SetFilePointerEx */
	NULL, /* FileLockFile */
	NULL, /* FileLockFileEx */
	NULL, /* FileUnlockFile */
	NULL, /* FileUnlockFileEx */
	NULL, /* SetFileTime */
	NULL, /* FileGetFileInformationByHandle */
};

static HANDLE
NamedPipeClientCreateFileA(LPCSTR lpFileName, WINPR_ATTR_UNUSED DWORD dwDesiredAccess,
                           WINPR_ATTR_UNUSED DWORD dwShareMode,
                           WINPR_ATTR_UNUSED LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                           WINPR_ATTR_UNUSED DWORD dwCreationDisposition,
                           DWORD dwFlagsAndAttributes, WINPR_ATTR_UNUSED HANDLE hTemplateFile)
{
	int status = 0;
	struct sockaddr_un s = { 0 };

	if (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED)
	{
		WLog_ERR(TAG, "WinPR does not support the FILE_FLAG_OVERLAPPED flag");
		SetLastError(ERROR_NOT_SUPPORTED);
		return INVALID_HANDLE_VALUE;
	}

	if (!lpFileName)
		return INVALID_HANDLE_VALUE;

	if (!IsNamedPipeFileNameA(lpFileName))
		return INVALID_HANDLE_VALUE;

	WINPR_NAMED_PIPE* pNamedPipe = (WINPR_NAMED_PIPE*)calloc(1, sizeof(WINPR_NAMED_PIPE));

	if (!pNamedPipe)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}

	HANDLE hNamedPipe = (HANDLE)pNamedPipe;
	WINPR_HANDLE_SET_TYPE_AND_MODE(pNamedPipe, HANDLE_TYPE_NAMED_PIPE, WINPR_FD_READ);
	pNamedPipe->name = _strdup(lpFileName);

	if (!pNamedPipe->name)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto fail;
	}

	pNamedPipe->dwOpenMode = 0;
	pNamedPipe->dwPipeMode = 0;
	pNamedPipe->nMaxInstances = 0;
	pNamedPipe->nOutBufferSize = 0;
	pNamedPipe->nInBufferSize = 0;
	pNamedPipe->nDefaultTimeOut = 0;
	pNamedPipe->dwFlagsAndAttributes = dwFlagsAndAttributes;
	pNamedPipe->lpFileName = GetNamedPipeNameWithoutPrefixA(lpFileName);

	if (!pNamedPipe->lpFileName)
		goto fail;

	pNamedPipe->lpFilePath = GetNamedPipeUnixDomainSocketFilePathA(lpFileName);

	if (!pNamedPipe->lpFilePath)
		goto fail;

	pNamedPipe->clientfd = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (pNamedPipe->clientfd < 0)
		goto fail;

	pNamedPipe->serverfd = -1;
	pNamedPipe->ServerMode = FALSE;
	s.sun_family = AF_UNIX;
	(void)sprintf_s(s.sun_path, ARRAYSIZE(s.sun_path), "%s", pNamedPipe->lpFilePath);
	status = connect(pNamedPipe->clientfd, (struct sockaddr*)&s, sizeof(struct sockaddr_un));
	pNamedPipe->common.ops = &ops;

	if (status != 0)
		goto fail;

	if (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED)
	{
		// TODO: Implement
		WLog_ERR(TAG, "TODO: implement this");
	}

	return hNamedPipe;

fail:
	if (pNamedPipe)
	{
		if (pNamedPipe->clientfd >= 0)
			close(pNamedPipe->clientfd);
		free(pNamedPipe->name);
		free(pNamedPipe->lpFileName);
		free(pNamedPipe->lpFilePath);
		free(pNamedPipe);
	}
	return INVALID_HANDLE_VALUE;
}

const HANDLE_CREATOR* GetNamedPipeClientHandleCreator(void)
{
	static const HANDLE_CREATOR NamedPipeClientHandleCreator = { .IsHandled = IsNamedPipeFileNameA,
		                                                         .CreateFileA =
		                                                             NamedPipeClientCreateFileA };
	return &NamedPipeClientHandleCreator;
}

#endif

/* Extended API */

#define NAMED_PIPE_PREFIX_PATH "\\\\.\\pipe\\"

BOOL IsNamedPipeFileNameA(LPCSTR lpName)
{
	if (strncmp(lpName, NAMED_PIPE_PREFIX_PATH, sizeof(NAMED_PIPE_PREFIX_PATH) - 1) != 0)
		return FALSE;

	return TRUE;
}

char* GetNamedPipeNameWithoutPrefixA(LPCSTR lpName)
{
	char* lpFileName = NULL;

	if (!lpName)
		return NULL;

	if (!IsNamedPipeFileNameA(lpName))
		return NULL;

	lpFileName = _strdup(&lpName[strnlen(NAMED_PIPE_PREFIX_PATH, sizeof(NAMED_PIPE_PREFIX_PATH))]);
	return lpFileName;
}

char* GetNamedPipeUnixDomainSocketBaseFilePathA(void)
{
	char* lpTempPath = NULL;
	char* lpPipePath = NULL;
	lpTempPath = GetKnownPath(KNOWN_PATH_TEMP);

	if (!lpTempPath)
		return NULL;

	lpPipePath = GetCombinedPath(lpTempPath, ".pipe");
	free(lpTempPath);
	return lpPipePath;
}

char* GetNamedPipeUnixDomainSocketFilePathA(LPCSTR lpName)
{
	char* lpPipePath = NULL;
	char* lpFileName = NULL;
	char* lpFilePath = NULL;
	lpPipePath = GetNamedPipeUnixDomainSocketBaseFilePathA();
	lpFileName = GetNamedPipeNameWithoutPrefixA(lpName);
	lpFilePath = GetCombinedPath(lpPipePath, lpFileName);
	free(lpPipePath);
	free(lpFileName);
	return lpFilePath;
}

int GetNamePipeFileDescriptor(HANDLE hNamedPipe)
{
#ifndef _WIN32
	int fd = 0;
	WINPR_NAMED_PIPE* pNamedPipe = (WINPR_NAMED_PIPE*)hNamedPipe;

	if (!NamedPipeClientIsHandled(hNamedPipe))
		return -1;

	fd = (pNamedPipe->ServerMode) ? pNamedPipe->serverfd : pNamedPipe->clientfd;
	return fd;
#else
	return -1;
#endif
}
