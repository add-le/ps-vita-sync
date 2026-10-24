// Benchmark harness shared by every stack. See ../README.md for the scripted run.
#include "harness.h"

#include <SDL_ttf.h>
#include <malloc.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef __vita__
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#define ASSETS "app0:/"
#define OUT_DIR "ux0:data/SYNCBENCH/"
// Same explicit heap for every stack. 128 MB is also newlib's default (vitasdk/newlib sbrk.c).
extern "C" unsigned int _newlib_heap_size_user = 128 * 1024 * 1024;
#else
#include <sys/stat.h>
#ifndef ASSETS
#define ASSETS "assets/"
#endif
#define OUT_DIR "results-host/"
#endif

namespace {

constexpr int WIDTH = 960;
constexpr int HEIGHT = 544;

// Scripted run, in seconds. Fixed so every stack gets the same workload.
constexpr double IDLE_END = 2.0;
constexpr double SCROLL_END = IDLE_END + 10.0;
constexpr double TRANSITION_END = SCROLL_END + 6.0;
constexpr double SCROLL_AMPLITUDE = 800.0;  // px
constexpr double SCROLL_PERIOD = 2.0;       // s, triangle wave

const char *PHASES[] = {"idle", "scroll", "transition", "interactive"};

// Scripted input during the idle phase, identical for every stack. Tile 2 is the "data" folder
// in the root listing; its centre is at (583, 83) with the shared 4-column layout.
struct InputStep {
  double t;
  enum { Press, Tap, Check } kind;
  bench::Button button;
  float x, y;
  const char *check;  // for Check: what is verified
  int focus, depth;   // expected values, -1 = not checked
};
const InputStep INPUT_SCRIPT[] = {
  {0.50, InputStep::Press, bench::Button::Down, 0, 0, nullptr, -1, -1},
  {0.70, InputStep::Press, bench::Button::Right, 0, 0, nullptr, -1, -1},
  {0.90, InputStep::Check, bench::Button::Up, 0, 0, "dpad_focus (Down, Right from tile 0)", 5, 0},
  {1.00, InputStep::Tap, bench::Button::Up, 583, 83, nullptr, -1, -1},
  {1.20, InputStep::Check, bench::Button::Up, 0, 0, "front_tap_opens_folder", -1, 1},
  {1.40, InputStep::Press, bench::Button::Circle, 0, 0, nullptr, -1, -1},
  {1.60, InputStep::Check, bench::Button::Up, 0, 0, "circle_goes_back", -1, 0},
};

#ifdef __vita__
double process_seconds() { return sceKernelGetProcessTimeWide() / 1e6; }
#else
const auto g_process_start = std::chrono::steady_clock::now();
double process_seconds() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_process_start).count();
}
#endif

size_t heap_used() {
#ifdef __vita__
  struct mallinfo mi = mallinfo();
  return (size_t)mi.uordblks;
#else
  struct mallinfo2 mi = mallinfo2();
  return mi.uordblks + mi.hblkhd;  // hblkhd: large blocks served by mmap
#endif
}

std::string free_memory() {
#ifdef __vita__
  SceKernelFreeMemorySizeInfo info;
  info.size = sizeof(info);
  sceKernelGetFreeMemorySize(&info);
  char buf[128];
  snprintf(buf, sizeof(buf), "user=%d cdram=%d phycont=%d", info.size_user, info.size_cdram,
           info.size_phycont);
  return buf;
#else
  return "n/a";
#endif
}

struct Frame {
  int phase;
  double t, interval_ms, work_ms;
  size_t heap;
};

double percentile(std::vector<double> v, double p) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  size_t i = (size_t)std::min<double>(v.size() - 1, std::floor(p * (v.size() - 1) + 0.5));
  return v[i];
}

double triangle(double t) {
  double x = std::fmod(t, SCROLL_PERIOD) / SCROLL_PERIOD;
  return x < 0.5 ? x * 2 : 2 - x * 2;
}

bool map_button(int sdl_button, bench::Button &out) {
  // SDL2 Vita joystick order: src/joystick/vita/SDL_sysjoystick.c
  static const bench::Button map[] = {
    bench::Button::Triangle, bench::Button::Circle, bench::Button::Cross, bench::Button::Square,
    bench::Button::L, bench::Button::R, bench::Button::Down, bench::Button::Left,
    bench::Button::Up, bench::Button::Right, bench::Button::Select, bench::Button::Start,
  };
  if (sdl_button < 0 || sdl_button >= (int)(sizeof(map) / sizeof(map[0]))) return false;
  out = map[sdl_button];
  return true;
}

bool map_key(SDL_Keycode k, bench::Button &out) {
  switch (k) {
    case SDLK_UP: out = bench::Button::Up; return true;
    case SDLK_DOWN: out = bench::Button::Down; return true;
    case SDLK_LEFT: out = bench::Button::Left; return true;
    case SDLK_RIGHT: out = bench::Button::Right; return true;
    case SDLK_RETURN: out = bench::Button::Cross; return true;
    case SDLK_BACKSPACE: out = bench::Button::Circle; return true;
    case SDLK_TAB: out = bench::Button::Triangle; return true;
    case SDLK_SPACE: out = bench::Button::Square; return true;
    case SDLK_F1: out = bench::Button::Select; return true;
    case SDLK_ESCAPE: out = bench::Button::Start; return true;
    default: return false;
  }
}

class Overlay {
public:
  void init(SDL_Renderer *r, const std::string &assets) {
    renderer = r;
    font = TTF_OpenFont((assets + "Kanit-Regular.ttf").c_str(), 14);
  }
  void set(const std::string &text) {
    if (!font) return;
    if (texture) SDL_DestroyTexture(texture);
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color{255, 255, 255, 255});
    texture = SDL_CreateTextureFromSurface(renderer, s);
    w = s->w;
    h = s->h;
    SDL_FreeSurface(s);
  }
  void draw() {
    if (!texture || hidden) return;
    SDL_Rect bg{WIDTH - w - 12, HEIGHT - h - 8, w + 8, h + 4};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    SDL_RenderFillRect(renderer, &bg);
    SDL_Rect dst{bg.x + 4, bg.y + 2, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
  }
  bool hidden = false;

private:
  SDL_Renderer *renderer = nullptr;
  TTF_Font *font = nullptr;
  SDL_Texture *texture = nullptr;
  int w = 0, h = 0;
};

// Rear-panel scrolling with momentum, shared by all stacks.
class RearScroll {
public:
  void down(float y, double now) { active = true; last_y = y; last_t = now; velocity = 0; }
  void move(float y, double now, bench::Stack &s) {
    if (!active) return;
    float dy = y - last_y;
    double dt = std::max(1e-3, now - last_t);
    s.scroll_by(-dy);
    velocity = 0.7 * velocity + 0.3 * (-dy / dt);
    last_y = y;
    last_t = now;
  }
  void up() { active = false; }
  void update(double dt, bench::Stack &s) {
    if (active || std::fabs(velocity) < 5) return;
    s.scroll_by((float)(velocity * dt));
    velocity *= std::exp(-4.0 * dt);
  }

private:
  bool active = false;
  float last_y = 0;
  double last_t = 0, velocity = 0;
};

void save_screenshot(SDL_Renderer *r, const std::string &path) {
#ifndef __vita__
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
  if (SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
    SDL_SaveBMP(s, path.c_str());
  SDL_FreeSurface(s);
#else
  (void)r;
  (void)path;
#endif
}

}  // namespace

int main(int, char **) {
  const char *env_assets = getenv("BENCH_ASSETS");
  std::string assets = env_assets ? env_assets : ASSETS;
  bool auto_exit = getenv("BENCH_AUTOEXIT") != nullptr;

  size_t heap_start = heap_used();
  std::string mem_start = free_memory();

#if defined(BENCH_RENDER_DRIVER)
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, BENCH_RENDER_DRIVER);
#elif defined(__vita__)
  // Pin the GXM renderer (precompiled shaders). The GLES2 renderer would need libshacccg.suprx.
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "VITA gxm");
#endif
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) return 1;
  TTF_Init();
  SDL_Window *window = SDL_CreateWindow("ui-bench", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WIDTH, HEIGHT, 0);
  Uint32 flags = SDL_RENDERER_ACCELERATED;
#ifdef __vita__
  flags |= SDL_RENDERER_PRESENTVSYNC;
#endif
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, flags);
  if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  SDL_RendererInfo rinfo;
  SDL_GetRendererInfo(renderer, &rinfo);
  if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);

  Overlay overlay;
  overlay.init(renderer, assets);
  size_t heap_before_stack = heap_used();

  bench::Stack *stack = bench::create_stack();
  double init_start = process_seconds();
  if (!stack->init(renderer, WIDTH, HEIGHT, assets.c_str())) {
    SDL_Log("stack init failed");
    return 1;
  }
  double init_ms = (process_seconds() - init_start) * 1000;
  size_t heap_after_init = heap_used();
  std::string mem_after_init = free_memory();

  std::string out_dir = OUT_DIR;
#ifdef __vita__
  sceIoMkdir("ux0:data/SYNCBENCH", 0777);
#else
  mkdir(out_dir.c_str(), 0777);
#endif
  std::string base = out_dir + stack->name();

  std::vector<std::string> checks;
  size_t next_input = 0;
  double tap_release = -1;
  std::vector<Frame> frames;
  frames.reserve(4096);
  size_t heap_peak = heap_after_init;
  double startup_ms = -1;
  double run_start = -1, last_present = -1, last_overlay = 0;
  double scroll_pos = 0;
  int toggles_done = 0;
  bool shot_focus = false, shot_nav = false, shot_idle = false, shot_header = false, running = true, script_done = false;
  RearScroll rear;

  auto write_results = [&]() {
    FILE *f = fopen((base + ".txt").c_str(), "w");
    if (f) {
      fprintf(f, "stack: %s\nplatform: %s\nrenderer: %s\n", stack->name(),
#ifdef __vita__
              "vita",
#else
              "host",
#endif
              rinfo.name);
#ifdef __vita__
      fprintf(f, "newlib_heap_bytes: %u\n", _newlib_heap_size_user);
#endif
      fprintf(f, "startup_ms (process start -> first frame presented): %.1f\n", startup_ms);
      fprintf(f, "stack_init_ms: %.1f\n", init_ms);
      fprintf(f, "heap_bytes: start=%zu before_stack=%zu after_init=%zu peak=%zu\n", heap_start,
              heap_before_stack, heap_after_init, heap_peak);
      fprintf(f, "free_memory: start[%s] after_init[%s] end[%s]\n", mem_start.c_str(),
              mem_after_init.c_str(), free_memory().c_str());
      fprintf(f, "script_completed: %s\n", script_done ? "yes" : "no");
      for (auto &c : checks) fprintf(f, "input_check %s\n", c.c_str());
      for (int p = 0; p < 4; p++) {
        std::vector<double> iv, wk;
        for (auto &fr : frames)
          if (fr.phase == p) { iv.push_back(fr.interval_ms); wk.push_back(fr.work_ms); }
        if (iv.empty()) continue;
        double sum = 0;
        for (double v : iv) sum += v;
        fprintf(f,
                "phase %-11s frames=%zu fps=%.1f interval_ms p50=%.2f p95=%.2f p99=%.2f max=%.2f"
                " work_ms p50=%.2f p95=%.2f p99=%.2f max=%.2f\n",
                PHASES[p], iv.size(), 1000.0 * iv.size() / sum, percentile(iv, .5), percentile(iv, .95),
                percentile(iv, .99), percentile(iv, 1), percentile(wk, .5), percentile(wk, .95),
                percentile(wk, .99), percentile(wk, 1));
      }
      fclose(f);
    }
    FILE *csv = fopen((base + ".csv").c_str(), "w");
    if (csv) {
      fprintf(csv, "phase,t,interval_ms,work_ms,heap\n");
      for (auto &fr : frames)
        fprintf(csv, "%s,%.4f,%.3f,%.3f,%zu\n", PHASES[fr.phase], fr.t, fr.interval_ms, fr.work_ms, fr.heap);
      fclose(csv);
    }
  };

  while (running) {
    double frame_start = process_seconds();
    if (run_start < 0) run_start = frame_start;
    double t = frame_start - run_start;
    int phase = t < IDLE_END ? 0 : t < SCROLL_END ? 1 : t < TRANSITION_END ? 2 : 3;
    double dt = last_present < 0 ? 0 : frame_start - last_present;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      bench::Button b;
      switch (e.type) {
        case SDL_QUIT: running = false; break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
          if (!map_button(e.jbutton.button, b)) break;
          if (b == bench::Button::Start && e.type == SDL_JOYBUTTONDOWN) running = false;
          else if (b == bench::Button::Select && e.type == SDL_JOYBUTTONDOWN) overlay.hidden = !overlay.hidden;
          else stack->button(b, e.type == SDL_JOYBUTTONDOWN);
          break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          if (!map_key(e.key.keysym.sym, b)) break;
          if (b == bench::Button::Start && e.type == SDL_KEYDOWN) running = false;
          else if (b == bench::Button::Select && e.type == SDL_KEYDOWN) overlay.hidden = !overlay.hidden;
          else stack->button(b, e.type == SDL_KEYDOWN);
          break;
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP: {
          // Vita: touch id 1 = front, 2 = rear (src/video/vita/SDL_vitatouch.c)
          if (e.tfinger.touchId == 2) {
            float y = e.tfinger.y * HEIGHT;
            if (e.type == SDL_FINGERDOWN) rear.down(y, frame_start);
            else if (e.type == SDL_FINGERMOTION) rear.move(y, frame_start, *stack);
            else rear.up();
          } else if (e.tfinger.touchId == 1) {
            bench::Touch::Kind k = e.type == SDL_FINGERDOWN ? bench::Touch::Down
                                 : e.type == SDL_FINGERUP ? bench::Touch::Up : bench::Touch::Move;
            stack->front_touch({k, (long long)e.tfinger.fingerId, e.tfinger.x * WIDTH, e.tfinger.y * HEIGHT});
          }
          break;
        }
#ifndef __vita__
        // Host: mouse = front touch, wheel = rear touch.
        case SDL_MOUSEBUTTONDOWN:
          stack->front_touch({bench::Touch::Down, 0, (float)e.button.x, (float)e.button.y});
          break;
        case SDL_MOUSEBUTTONUP:
          stack->front_touch({bench::Touch::Up, 0, (float)e.button.x, (float)e.button.y});
          break;
        case SDL_MOUSEMOTION:
          if (e.motion.state & SDL_BUTTON_LMASK)
            stack->front_touch({bench::Touch::Move, 0, (float)e.motion.x, (float)e.motion.y});
          break;
        case SDL_MOUSEWHEEL: stack->scroll_by(-e.wheel.y * 60.0f); break;
#endif
      }
    }

    // Scripted input checks.
    if (tap_release >= 0 && t >= tap_release) {
      stack->front_touch({bench::Touch::Up, 99, 583, 83});
      tap_release = -1;
    }
    while (next_input < sizeof(INPUT_SCRIPT) / sizeof(INPUT_SCRIPT[0]) && t >= INPUT_SCRIPT[next_input].t) {
      const InputStep &s = INPUT_SCRIPT[next_input++];
      if (s.kind == InputStep::Press) {
        stack->button(s.button, true);
        stack->button(s.button, false);
      } else if (s.kind == InputStep::Tap) {
        stack->front_touch({bench::Touch::Down, 99, s.x, s.y});
        tap_release = t + 0.05;
      } else {
        int focus = stack->focused_index(), depth = stack->folder_depth();
        bool ok = (s.focus < 0 || focus == s.focus) && (s.depth < 0 || depth == s.depth);
        const char *verdict = focus == -2 ? "n/a" : ok ? "PASS" : "FAIL";
        char buf[200];
        snprintf(buf, sizeof(buf), "%s: %s (focus=%d depth=%d)", s.check, verdict, focus, depth);
        checks.push_back(buf);
      }
    }

    // Scripted workload.
    if (phase == 1) {
      double target = SCROLL_AMPLITUDE * triangle(t - IDLE_END);
      stack->scroll_by((float)(target - scroll_pos));
      scroll_pos = target;
    } else if (phase == 2) {
      if (scroll_pos != 0) { stack->scroll_by((float)-scroll_pos); scroll_pos = 0; }
      int due = (int)(t - SCROLL_END) + 1;  // one toggle per second
      while (toggles_done < due && toggles_done < 6) { stack->toggle_select(1); toggles_done++; }
    }
    rear.update(dt, *stack);

    stack->update(t);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    stack->render();
    overlay.draw();
    double work_end = process_seconds();

    if (!shot_focus && t > 0.85) { save_screenshot(renderer, base + "-focus.bmp"); shot_focus = true; }
    if (!shot_nav && t > 1.15) { save_screenshot(renderer, base + "-nav.bmp"); shot_nav = true; }
    if (!shot_idle && t > IDLE_END - 0.1) { save_screenshot(renderer, base + "-idle.bmp"); shot_idle = true; }
    if (!shot_header && t > SCROLL_END + 0.9) { save_screenshot(renderer, base + "-header.bmp"); shot_header = true; }

    SDL_RenderPresent(renderer);
    double present = process_seconds();
    if (startup_ms < 0) startup_ms = present * 1000;

    size_t heap = heap_used();
    heap_peak = std::max(heap_peak, heap);
    if (last_present >= 0)
      frames.push_back({phase, t, (present - last_present) * 1000, (work_end - frame_start) * 1000, heap});
    last_present = present;

    if (present - last_overlay > 0.5 && frames.size() > 2) {
      double sum = 0;
      int n = 0;
      for (size_t i = frames.size(); i-- > 0 && n < 30; n++) sum += frames[i].interval_ms;
      char buf[160];
      snprintf(buf, sizeof(buf), "%s  %.1f fps  work %.1f ms  heap %.1f MB  %s", stack->name(),
               1000.0 * n / sum, frames.back().work_ms, heap / 1048576.0, PHASES[phase]);
      overlay.set(buf);
      last_overlay = present;
    }

    if (phase == 3 && !script_done) {
      script_done = true;
      // Written now as well as at exit, so a crash in the interactive phase loses nothing.
      write_results();
      if (auto_exit) running = false;
    }
  }
  write_results();

  delete stack;
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  return 0;
}