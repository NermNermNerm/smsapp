#pragma once

class FakeKdeConnectDaemon;

namespace Commands {
// Start the interactive shell; the daemon pointer is used for actions.
void startInteractiveShell(FakeKdeConnectDaemon *daemon);

}
