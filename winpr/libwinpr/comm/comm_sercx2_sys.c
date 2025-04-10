/**
 * WinPR: Windows Portable Runtime
 * Serial Communication API
 *
 * Copyright 2011 O.S. Systems Software Ltda.
 * Copyright 2011 Eduardo Fiss Beloni <beloni@ossystems.com.br>
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2014 Hewlett-Packard Development Company, L.P.
 *
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

#include <winpr/assert.h>
#include <winpr/wlog.h>

#include "comm_serial_sys.h"
#include "comm_sercx_sys.h"

#include "comm_sercx2_sys.h"

/* http://msdn.microsoft.com/en-us/library/dn265347%28v=vs.85%29.aspx
 *
 * SerCx2 does not support special characters. SerCx2 always completes
 * an IOCTL_SERIAL_SET_CHARS request with a STATUS_SUCCESS status
 * code, but does not set any special characters or perform any other
 * operation in response to this request. For an
 * IOCTL_SERIAL_GET_CHARS request, SerCx2 sets all the character
 * values in the SERIAL_CHARS structure to null, and completes the
 * request with a STATUS_SUCCESS status code.
 */

static BOOL set_serial_chars(WINPR_ATTR_UNUSED WINPR_COMM* pComm,
                             WINPR_ATTR_UNUSED const SERIAL_CHARS* pSerialChars)
{
	WINPR_ASSERT(pComm);
	WINPR_ASSERT(pSerialChars);

	return TRUE;
}

static BOOL get_serial_chars(WINPR_ATTR_UNUSED WINPR_COMM* pComm, SERIAL_CHARS* pSerialChars)
{
	WINPR_ASSERT(pComm);
	WINPR_ASSERT(pSerialChars);

	ZeroMemory(pSerialChars, sizeof(SERIAL_CHARS));
	return TRUE;
}

/* http://msdn.microsoft.com/en-us/library/windows/hardware/hh439605%28v=vs.85%29.aspx */
/* FIXME: only using the Serial.sys' events, complete the support of the remaining events */
static const ULONG SERCX2_SYS_SUPPORTED_EV_MASK =
    SERIAL_EV_RXCHAR | SERIAL_EV_RXFLAG | SERIAL_EV_TXEMPTY | SERIAL_EV_CTS | SERIAL_EV_DSR |
    SERIAL_EV_RLSD | SERIAL_EV_BREAK | SERIAL_EV_ERR | SERIAL_EV_RING |
    /* SERIAL_EV_PERR     | */
    SERIAL_EV_RX80FULL /*|
    SERIAL_EV_EVENT1   |
    SERIAL_EV_EVENT2*/
    ;

/* use Serial.sys for basis (not SerCx.sys) */
static BOOL set_wait_mask(WINPR_COMM* pComm, const ULONG* pWaitMask)
{
	const SERIAL_DRIVER* pSerialSys = SerialSys_s();

	WINPR_ASSERT(pComm);
	WINPR_ASSERT(pWaitMask);
	WINPR_ASSERT(pSerialSys);

	const ULONG possibleMask = *pWaitMask & SERCX2_SYS_SUPPORTED_EV_MASK;

	if (possibleMask != *pWaitMask)
	{
		CommLog_Print(WLOG_WARN,
		              "Not all wait events supported (SerCx2.sys), requested events= 0x%08" PRIX32
		              ", possible events= 0x%08" PRIX32 "",
		              *pWaitMask, possibleMask);

		/* FIXME: shall we really set the possibleMask and return FALSE? */
		pComm->WaitEventMask = possibleMask;
		return FALSE;
	}

	/* NB: All events that are supported by SerCx.sys are supported by Serial.sys*/
	return pSerialSys->set_wait_mask(pComm, pWaitMask);
}

static BOOL purge(WINPR_COMM* pComm, const ULONG* pPurgeMask)
{
	const SERIAL_DRIVER* pSerialSys = SerialSys_s();

	WINPR_ASSERT(pComm);
	WINPR_ASSERT(pPurgeMask);
	WINPR_ASSERT(pSerialSys);

	/* http://msdn.microsoft.com/en-us/library/windows/hardware/ff546655%28v=vs.85%29.aspx */

	if ((*pPurgeMask & SERIAL_PURGE_RXCLEAR) && !(*pPurgeMask & SERIAL_PURGE_RXABORT))
	{
		CommLog_Print(WLOG_WARN,
		              "Expecting SERIAL_PURGE_RXABORT since SERIAL_PURGE_RXCLEAR is set");
		SetLastError(ERROR_INVALID_DEVICE_OBJECT_PARAMETER);
		return FALSE;
	}

	if ((*pPurgeMask & SERIAL_PURGE_TXCLEAR) && !(*pPurgeMask & SERIAL_PURGE_TXABORT))
	{
		CommLog_Print(WLOG_WARN,
		              "Expecting SERIAL_PURGE_TXABORT since SERIAL_PURGE_TXCLEAR is set");
		SetLastError(ERROR_INVALID_DEVICE_OBJECT_PARAMETER);
		return FALSE;
	}

	return pSerialSys->purge(pComm, pPurgeMask);
}

/* specific functions only */
static SERIAL_DRIVER SerCx2Sys = {
	.id = SerialDriverSerCx2Sys,
	.name = _T("SerCx2.sys"),
	.set_baud_rate = NULL,
	.get_baud_rate = NULL,
	.get_properties = NULL,
	.set_serial_chars = set_serial_chars,
	.get_serial_chars = get_serial_chars,
	.set_line_control = NULL,
	.get_line_control = NULL,
	.set_handflow = NULL,
	.get_handflow = NULL,
	.set_timeouts = NULL,
	.get_timeouts = NULL,
	.set_dtr = NULL,
	.clear_dtr = NULL,
	.set_rts = NULL,
	.clear_rts = NULL,
	.get_modemstatus = NULL,
	.set_wait_mask = set_wait_mask,
	.get_wait_mask = NULL,
	.wait_on_mask = NULL,
	.set_queue_size = NULL,
	.purge = purge,
	.get_commstatus = NULL,
	.set_break_on = NULL,
	.set_break_off = NULL,
	.set_xoff = NULL, /* not supported by SerCx2.sys */
	.set_xon = NULL,  /* not supported by SerCx2.sys */
	.get_dtrrts = NULL,
	.config_size = NULL,    /* not supported by SerCx2.sys */
	.immediate_char = NULL, /* not supported by SerCx2.sys */
	.reset_device = NULL,   /* not supported by SerCx2.sys */
};

const SERIAL_DRIVER* SerCx2Sys_s(void)
{
	/* SerCx2Sys completed with inherited functions from SerialSys or SerCxSys */
	const SERIAL_DRIVER* pSerialSys = SerialSys_s();
	const SERIAL_DRIVER* pSerCxSys = SerCxSys_s();
	if (!pSerialSys || !pSerCxSys)
		return NULL;

	SerCx2Sys.set_baud_rate = pSerialSys->set_baud_rate;
	SerCx2Sys.get_baud_rate = pSerialSys->get_baud_rate;

	SerCx2Sys.get_properties = pSerialSys->get_properties;

	SerCx2Sys.set_line_control = pSerCxSys->set_line_control;
	SerCx2Sys.get_line_control = pSerCxSys->get_line_control;

	/* Only SERIAL_CTS_HANDSHAKE, SERIAL_RTS_CONTROL and SERIAL_RTS_HANDSHAKE flags are really
	 * required by SerCx2.sys http://msdn.microsoft.com/en-us/library/jj680685%28v=vs.85%29.aspx
	 */
	SerCx2Sys.set_handflow = pSerialSys->set_handflow;
	SerCx2Sys.get_handflow = pSerialSys->get_handflow;

	SerCx2Sys.set_timeouts = pSerialSys->set_timeouts;
	SerCx2Sys.get_timeouts = pSerialSys->get_timeouts;

	SerCx2Sys.set_dtr = pSerialSys->set_dtr;
	SerCx2Sys.clear_dtr = pSerialSys->clear_dtr;

	SerCx2Sys.set_rts = pSerialSys->set_rts;
	SerCx2Sys.clear_rts = pSerialSys->clear_rts;

	SerCx2Sys.get_modemstatus = pSerialSys->get_modemstatus;

	SerCx2Sys.set_wait_mask = pSerialSys->set_wait_mask;
	SerCx2Sys.get_wait_mask = pSerialSys->get_wait_mask;
	SerCx2Sys.wait_on_mask = pSerialSys->wait_on_mask;

	SerCx2Sys.set_queue_size = pSerialSys->set_queue_size;

	SerCx2Sys.get_commstatus = pSerialSys->get_commstatus;

	SerCx2Sys.set_break_on = pSerialSys->set_break_on;
	SerCx2Sys.set_break_off = pSerialSys->set_break_off;

	SerCx2Sys.get_dtrrts = pSerialSys->get_dtrrts;

	return &SerCx2Sys;
}
