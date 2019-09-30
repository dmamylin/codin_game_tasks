# About
Link: https://www.codingame.com/ide/puzzle/power-of-thor-episode-2

The idea is very simple:
1. If Thor doesn't have any safe positions to move OR he can strike the most distant giant, we return "STRIKE" command. **GOTO step 1.**
2. Iterate over safe paths (we say that a path is safe if there are no giants who can reach any point of that path exactly in 1 turn) and compute safe distance between Thor and the most distant giant. To do so we can use BFS.
3. If there is no any safe path, just calculate distance between Thor and the most distant giant and pick any optimal direction. Otherwise pick optimal direction according to safe path. **GOTO step 1.**
