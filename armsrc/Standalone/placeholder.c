#include "standalone.h" // standalone definitions

#include "dbprint.h"

void ModInfo(void) {
    DbpString("  No standalone mode present");
}

void RunMod(void) {
}

void StandaloneReplyStatus() {
    reply_ng(CMD_GET_STANDALONE_DONE_STATUS, PM3_EUNDEF, NULL, 0);
}
