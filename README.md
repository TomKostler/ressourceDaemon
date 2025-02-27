# ressourceDaemon

A MAC OS system daemon useful for tracking the systems CPU, RAM and disc usage.

# Usage
- Call `make` in the directory
- Run `./ressourceDaemond cpu ram disc` with the arguments you want to track.

If the usage of the tracked resources (CPU, RAM, or disk) exceeds a threshold, a notification is sent.
