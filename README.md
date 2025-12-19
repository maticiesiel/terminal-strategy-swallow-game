# 🦅 Terminal Swallow Strategy Game

A C/C++ survival game running natively in the Unix terminal. The player controls a swallow that must navigate a hostile environment, manage stamina, and utilize "Taxi" mechanics to survive against hunter algorithms.

## 🛠 Engineering Highlights
* **Memory Management:** Manual allocation/deallocation (`malloc`/`free`) for all game entities (Hunters, Stars).
* **Physics Engine:** Custom collision detection and vector-based movement for hunters.
* **Unix Integration:** Uses `ncurses` for rendering and POSIX signals for timing.
* **Data Persistence:** Reads/writes configuration and high scores to local files.

## 🎮 How to Run (Mac/Linux)
1. **Clone the repo:**
   ```bash
   git clone [https://github.com/maticiesiel/terminal-strategy-swallow-game.git](https://github.com/maticiesiel/terminal-strategy-swallow-game.git)
   cd terminal-strategy-swallow-game
