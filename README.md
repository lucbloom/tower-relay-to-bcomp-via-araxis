# Tower Relay to Beyond Compare via spoofed Araxis Merge install
A tiny C++ relay to fix a bug in Tower that makes it impossible to diff 2 sessions on Windows

Tower cannot seem to open 2 simultaneous sessions in Beyond Compare 4 when doing a diff without ending up in 2 "File Not Found: C\temp\3243234234_file.json" panes. This project installs itself as a fake install of Araxis Merge and relays Tower's diff and merge commands to Beyond Compare with the correct parameters for graet success! Wowiewowa!
