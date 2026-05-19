# 🚀 SECTOR 7 : PLANETARY DEFENSE

> A 2D space shooter built with OpenGL (C++) and ported to HTML5 Canvas — defend the L-5 Colony across 4 planetary levels.

---

## 🎮 Play (HTML5)

Open `sector7.html` in any modern browser. No install needed.

| Control | Action |
|---|---|
| `W A S D` / Arrow Keys | Move ship |
| `Space` | Fire |
| `Enter` | Start / Restart |
| `Esc` | Return to Menu |

---

## 🌍 Levels

| # | Level | Enemy | Ship |
|---|---|---|---|
| 1 | Earth Skies | Interceptor Drone | Prototype Fighter |
| 2 | Venus Assault | UFO Saucer | Aerospace Fighter |
| 3 | Mercury Strike | Demon Alien | Plasma Interceptor |
| 4 | Sun Titan Boss | Boss — Sun Titan | Haunted Solar Destroyer |

Each level has a scrolling parallax background unique to its planet.

---

## 🛠️ Tech Stack

**Original (C++ / OpenGL)**
- Legacy OpenGL with GLUT/freeGLUT
- `GL_POLYGON`, `GL_TRIANGLES`, `GL_QUADS` for all shapes
- Custom `drawPolyCircle` lambda helper for repeated circle drawing
- 60fps game loop via `glutTimerFunc`

**HTML5 Port**
- Canvas 2D API — zero dependencies, runs in-browser
- `requestAnimationFrame` game loop with delta-time
- Same coordinate system and game logic as the C++ version

---

## 🏗️ Project Structure

```
sector7/
├── main.cpp          # Original C++ / OpenGL source
├── sector7.html      # Self-contained HTML5 port
└── README.md
```

---

## ⚙️ Build (C++ Version)

**macOS**
```bash
g++ main.cpp -o sector7 -framework OpenGL -framework GLUT
./sector7
```

**Linux**
```bash
g++ main.cpp -o sector7 -lGL -lGLU -lglut
./sector7
```

**Windows** — link against `freeglut` and `opengl32`.

---

## 👥 Authors

**Group 01 — Computer Graphics, Section F**
