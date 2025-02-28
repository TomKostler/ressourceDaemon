# ressourceDaemond

A MAC OS system daemon useful for tracking the systems CPU, RAM and disc usage.
The functionality itself is implemented, while the daemonization process is left out for the user to decide whether to test it directly or run it as a real daemon using launchd.

# Usage
- Call `make` in the directory
- Run `./ressourceDaemond cpu ram disc` with the arguments you want to track.

If the usage of the tracked resources (CPU, RAM, or disk) exceeds a threshold, a notification is sent.
