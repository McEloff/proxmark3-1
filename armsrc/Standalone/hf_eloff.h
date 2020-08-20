//-----------------------------------------------------------------------------
// Mikhail Elov, 2019
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// StandAlone Mod
//-----------------------------------------------------------------------------

#ifndef __HF_ELOFF_H
#define __HF_ELOFF_H

#include <stdbool.h>
#include "standalone.h"
#include "appmain.h"
#include "mifaresim.h"
#include "mifareutil.h"
#include "mifare.h"
#include "iso14443a.h"
#include "BigBuf.h"
#include "cmd.h"
#include "dbprint.h"
#include "fpgaloader.h"
#include "lfsampling.h"
#include "proxmark3_arm.h"
#include "util.h"
#include "string.h"
#include "lfdemod.h"

void StandaloneReplyStatus(void);
uint32_t DemodLF(int trigger_threshold);
void WorkWithLF(void);
void WorkWithHF(void);

#endif /* __HF_ELOFF_H */
