#include <ncurses.h>
#include <locale.h>
#include <vector>
#include "window.hpp"

#include "../lib/vcd_parser/VCDFileParser.hpp"
#include "../lib/vcd_parser/VCDFile.hpp"
#include "../lib/vcd_parser/VCDTypes.hpp"

#define KEY_BKSPC 127

// Global var
VCDFile * global_trace = nullptr;

struct ExplorerWinObj
{
    void * obj = nullptr;
    bool is_signal = false;
};

class ExplorerWin: public Window
{
public:
    unsigned selected_line_indx = 0, max_line_cnt = 0;
    int selected_scope_indx = 0;
    std::vector<VCDScope*> scopes;
    std::vector<bool> scope_is_expanded;

    ExplorerWinObj selected_obj;

    ExplorerWin(int height, int width, int y, int x): Window(height, width, y, x, "Explorer")
    {
        scopes = *global_trace->get_scopes();
        
        // all scopes are not expanded initially
        for (unsigned i=0; i<scopes.size(); i++)
            scope_is_expanded.push_back(false);
    }

    virtual void print()
    {
        unsigned voffset = 1;
        unsigned line = 0;
        for(unsigned scope_indx=0; scope_indx < scopes.size(); scope_indx++)
        {
            // print scope name
            move(line+voffset, 1);
            
            bool scope_selected = (selected_line_indx == line);
            if (scope_selected)
            {
                selected_obj.obj = (void*) scopes[scope_indx];
                selected_obj.is_signal = false;
                selected_scope_indx = (int)scope_indx;
                wattron(win, COLOR_PAIR(1));
            }

            wattron(win, A_BOLD);
            const char* arrow = scope_is_expanded[scope_indx] ? "v" : ">";
            wprintw(win, " %s %s", arrow, (scope_indx==0 ? "*" : scopes[scope_indx]->name.c_str()));
            wattroff(win, A_BOLD);

            if (scope_selected)
                wattroff(win, COLOR_PAIR(1));

            line++;
            auto signals = scopes[scope_indx]->signals;

            if(scope_is_expanded[scope_indx])
            {
                for(unsigned sig_indx=0; sig_indx < signals.size(); sig_indx++)
                {
                    move(line+voffset, 1);
                    bool sig_selected = (selected_line_indx == line);
                    if (sig_selected)
                    {
                        selected_obj.obj = (void*) signals[sig_indx];
                        selected_obj.is_signal = true;
                        selected_scope_indx = -1;
                        wattron(win, COLOR_PAIR(1));
                    }
                    else
                    {
                        wattron(win, COLOR_PAIR(2));
                    }

                    wprintw(win, "   𜸅 %s [%d:0]", signals[sig_indx]->reference.c_str(), signals[sig_indx]->size);

                    if (sig_selected)
                        wattroff(win, COLOR_PAIR(1));
                    else
                        wattroff(win, COLOR_PAIR(2));

                    line++;
                }
            }
        }

        // save last line no
        max_line_cnt = line;
    }

    VCDSignal* get_selected_signal()
    {
        if(!selected_obj.is_signal)
            return nullptr;
        return (VCDSignal*) selected_obj.obj;
    }

    virtual void keypress(char key)
    {
        if (key == 'w')
        {
            if (selected_line_indx > 0)
                selected_line_indx--;
        }
        else if (key == 's')
        {
            if (selected_line_indx < max_line_cnt-1)
                selected_line_indx++;
        }
        else if (key == 'e')
        {
            if (!selected_obj.is_signal && selected_scope_indx >= 0)
                scope_is_expanded[selected_scope_indx] = !scope_is_expanded[selected_scope_indx];
        }

        // redraw
        print();
    }
};

class SignalsWin: public Window
{
public:
    unsigned selected_line_indx = 0; 
    unsigned max_line_cnt;
    std::vector<VCDSignal*> signals_;
    SignalsWin(int height, int width, int y, int x): Window(height, width, y, x, "Signals")
    {}

    void add_signal(VCDSignal * sig)
    {
        if(sig == nullptr)
            return;
        signals_.push_back(sig);
    }

    virtual void print() 
    {
        unsigned voffset = 2;
        for(unsigned sig_indx=0; sig_indx<signals_.size(); sig_indx++)
        {
            move(sig_indx + voffset, 1);

            int row_cpair = (sig_indx % 2 == 0) ? COLOR_PAIR(5) : COLOR_PAIR(6);
            wattron(win, selected_line_indx == sig_indx ? COLOR_PAIR(1) : row_cpair);
            wprintw(win, " %s [%d:0]", signals_[sig_indx]->reference.c_str(), signals_[sig_indx]->size);
            wattroff(win, selected_line_indx == sig_indx ? COLOR_PAIR(1) : row_cpair);
        }
        max_line_cnt = signals_.size();
    }

    virtual void keypress(char key)
    {
        if (key == 'w')
        {
            if (selected_line_indx > 0)
                selected_line_indx--;
        }
        else if (key == 's')
        {
            if (selected_line_indx < max_line_cnt-1)
                selected_line_indx++;
        }
        else if(key == KEY_BKSPC)
        {
            if(signals_.size() == 0)
                return;
            
            auto it = signals_.begin() + selected_line_indx;
            if (selected_line_indx == signals_.size()-1)
                selected_line_indx > 0 ? selected_line_indx--: 0;
            signals_.erase(it);
        }
        
        // redraw
        print();
    }
};

class MonitorWin: public Window
{
public:
    SignalsWin * signals_win = nullptr;

    unsigned zoom = 1;      // time units per display column (min 1)
    unsigned x_offset = 0;  // starting column index (pan)

    MonitorWin(int height, int width, int y, int x): Window(height, width, y, x, "Monitor")
    {}

    // Convert a bit vector to a hex string; returns "X"/"Z" if any bit is X/Z
    std::string vec_to_hex(VCDBitVector* vec)
    {
        for (VCDBit b : *vec) if (b == VCD_X) return "X";
        for (VCDBit b : *vec) if (b == VCD_Z) return "Z";
        unsigned long val = 0;
        for (VCDBit b : *vec) val = (val << 1) | (b == VCD_1 ? 1 : 0);
        char buf[32];
        snprintf(buf, sizeof(buf), "%lX", val);
        return std::string(buf);
    }

    virtual void print()
    {
        int cols = width - 2;
        if (cols <= 0) return;

        // Unicode drawing chars — ncursesw renders these as single-width cells
        static const char SIG_HIGH[] = "▇";  // U+2588  sustained high (full block)
        static const char SIG_LOW[]  = "▁";  // U+2581  sustained low
        static const char SIG_RISE[] = "▇";  // U+2589  rising edge  (left 7/8 block)
        static const char SIG_FALL[] = "▇";  // U+2589  falling edge (left 7/8 block)
        static const char SIG_X[]    = "🮘";  // U+2592  unknown (hatched)
        static const char SIG_Z[]    = "╌";  // U+254C  high-Z (dashed)
        static const char RUL_TICK[] = "┬";  // U+252C  ruler tick
        static const char RUL_FILL[] = "─";  // U+2500  ruler baseline
        static const char BUS_EDGE[] = "|";  // U+2589  bus transition bar (left 7/8 block)
        static const char BUS_FILL[] = "▓";  // U+2588  bus body fill

        // ── ruler ──────────────────────────────────────────────────────────
        move(1, 1);
        wattron(win, A_DIM);
        for (int x = 0; x < cols; x++)
        {
            unsigned k = (unsigned)x + x_offset;
            unsigned t = zoom * k;
            if (k % 10 == 0)
            {
                waddstr(win, RUL_TICK);
                x++;
                char tbuf[16];
                int tlen = snprintf(tbuf, sizeof(tbuf), "%u", t);
                for (int i = 0; i < tlen && x < cols; i++, x++)
                    waddch(win, tbuf[i]);
                x--;
            }
            else
            {
                waddstr(win, RUL_FILL);
            }
        }
        wattroff(win, A_DIM);

        // ── signals ────────────────────────────────────────────────────────
        unsigned voffset = 2;
        for (unsigned si = 0; si < signals_win->signals_.size(); si++)
        {
            move(si + voffset, 1);
            VCDSignal* sig = signals_win->signals_[si];
            bool is_scalar = (sig->size == 1);

            int sig_cpair = (si % 2 == 0) ? COLOR_PAIR(5) : COLOR_PAIR(6);

            VCDBit      prev_bit = VCD_X;
            std::string prev_hex;
            int         last_change_x = -1;
            std::string last_hex;

            for (int x = 0; x < cols; x++)
            {
                unsigned t = zoom * ((unsigned)x + x_offset);
                VCDValue* val = global_trace->get_signal_value_at(sig->hash, t);

                if (is_scalar)
                {
                    VCDBit curr = val->get_value_bit();

                    if (curr == VCD_X)
                    {
                        wattron(win, COLOR_PAIR(3) | A_BOLD);
                        waddstr(win, SIG_X);
                        wattroff(win, COLOR_PAIR(3) | A_BOLD);
                    }
                    else if (curr == VCD_Z)
                    {
                        wattron(win, COLOR_PAIR(4));
                        waddstr(win, SIG_Z);
                        wattroff(win, COLOR_PAIR(4));
                    }
                    else if (prev_bit == VCD_0 && curr == VCD_1)
                    {
                        wattron(win, sig_cpair | A_BOLD);
                        waddstr(win, SIG_RISE);
                        wattroff(win, sig_cpair | A_BOLD);
                    }
                    else if (prev_bit == VCD_1 && curr == VCD_0)
                    {
                        wattron(win, sig_cpair | A_BOLD);
                        waddstr(win, SIG_FALL);
                        wattroff(win, sig_cpair | A_BOLD);
                    }
                    else if (curr == VCD_1)
                    {
                        wattron(win, sig_cpair | A_BOLD);
                        waddstr(win, SIG_HIGH);
                        wattroff(win, sig_cpair | A_BOLD);
                    }
                    else  // VCD_0
                    {
                        wattron(win, sig_cpair);
                        waddstr(win, SIG_LOW);
                        wattroff(win, sig_cpair);
                    }
                    prev_bit = curr;
                }
                else  // multi-bit bus
                {
                    VCDBitVector* vec = val->get_value_vector();
                    std::string hex = vec_to_hex(vec);
                    bool changed = (hex != prev_hex);
                    if (changed) { last_change_x = x; last_hex = hex; prev_hex = hex; }

                    // color based on value state
                    int cpair = (last_hex == "X") ? COLOR_PAIR(3) :
                                (last_hex == "Z") ? COLOR_PAIR(4) : sig_cpair;
                    const char* fill = (last_hex == "X") ? SIG_X :
                                       (last_hex == "Z") ? SIG_Z : BUS_FILL;

                    int off = x - last_change_x;
                    if (off == 0)
                    {
                        wattron(win, cpair | A_BOLD);
                        waddstr(win, BUS_EDGE);
                        wattroff(win, cpair | A_BOLD);
                    }
                    else if (off - 1 < (int)last_hex.size())
                    {
                        wattron(win, cpair | A_BOLD);
                        waddch(win, last_hex[off - 1]);
                        wattroff(win, cpair | A_BOLD);
                    }
                    else
                    {
                        wattron(win, cpair | A_DIM);
                        waddstr(win, fill);
                        wattroff(win, cpair | A_DIM);
                    }
                }
            }
        }
    }

    unsigned max_zoom()
    {
        auto* times = global_trace->get_timestamps();
        if (times->empty()) return 1;
        int cols = std::max(1, width - 2);
        return std::max(1u, (unsigned)(times->back()) / (unsigned)cols);
    }

    virtual void keypress(char key)
    {
        if      (key == 'i') zoom = (zoom > 1) ? zoom / 2 : 1;
        else if (key == 'k') zoom = std::min(zoom * 2, max_zoom());
        else if (key == 'j') x_offset = (x_offset > 0) ? x_offset - 1 : 0;
        else if (key == 'l') x_offset++;
        print();
    }
};



int main(int argc, char** argv)
{
    if(argc<2)
    {
        printf("No VCD file provided\n");
        return -1;
    }
    std::string filepath(argv[1]);

    // parse file
    VCDFileParser parser;
    global_trace = parser.parse_file(filepath);


    // initialize program
    setlocale(LC_ALL, "");      // enable UTF-8 multibyte rendering
    initscr();                  // initialize ncurses mode
    cbreak();                   // disable getch() buffering
    noecho();                   // disable getch() echo
    nodelay(stdscr, TRUE);      // disable getch() waiting for keypress
    curs_set(0);                // Hide cursor

    if(!has_colors())
    {
        printw("Terminal doesn't support colors\n");
        return -1;
    }
    start_color();
    init_pair(2, COLOR_CYAN,    COLOR_BLACK);  // explorer signals
    init_pair(3, COLOR_RED,     COLOR_BLACK);  // VCD_X
    init_pair(4, COLOR_YELLOW,  COLOR_BLACK);  // VCD_Z
    init_pair(5, COLOR_GREEN,   COLOR_BLACK);  // waveform even
    init_pair(6, COLOR_CYAN,    COLOR_BLACK);  // waveform odd

    // get stdscr dimensions
    int stdscr_height, stdscr_width;
    getmaxyx(stdscr, stdscr_height, stdscr_width);

    // Print header
    std::string header = "*****   Terminal VCD viewer   *****";
    wmove(stdscr, 0, (stdscr_width/2) -(header.length()/2));    // center align
    wprintw(stdscr, "%s", header.c_str());

    // print footer
    wmove(stdscr, stdscr_height-1, 1);    // center align
    wprintw(stdscr, "q: quit\tw/a/s/d: move selection\te:expand/collapse\tenter:add signal\tbkspc:remove signal\ti/k: zoom\tj/l:pan");
    
    // Print filename
    wmove(stdscr, 1, 1);
    wprintw(stdscr, "File: %s", filepath.c_str());

    // Windows
    ExplorerWin win_explorer(stdscr_height-4, stdscr_width / 8, 3, 1);
    SignalsWin win_signals(stdscr_height-4, stdscr_width / 8, 3, win_explorer.width+1);
    int monitor_x = win_explorer.width + win_signals.width + 1;
    MonitorWin win_monitor(stdscr_height-4, stdscr_width - monitor_x - 1, 3, monitor_x);

    win_monitor.signals_win = &win_signals;
    
    // win list
    std::vector<Window *> avail_windows = {&win_explorer, &win_signals, &win_monitor};
    auto selected_win = avail_windows.begin();
    (*selected_win)->is_selected = true;

    wrefresh(stdscr);
    for(auto itr: avail_windows)
        itr->refresh();

    while (1)
    {
        // read input
        char inchar = getch();

        if(inchar == 'q')
        {
            break;
        }
        else if(inchar == 'd')  // select right window
        {
            if (selected_win != avail_windows.end()-1)
            {
                (*selected_win)->is_selected = false;
                (*selected_win)->refresh();
                selected_win++;
                (*selected_win)->is_selected = true;
                (*selected_win)->refresh();
            }
        }
        else if(inchar == 'a')  // select left window
        {
            if (selected_win != avail_windows.begin())
            {
                (*selected_win)->is_selected = false;
                (*selected_win)->refresh();
                selected_win--;
                (*selected_win)->is_selected = true;
                (*selected_win)->refresh();
            }
        }
        else if(inchar == 'w' || inchar == 's' || inchar == 'e' || inchar == KEY_BKSPC)
        {
            (*selected_win)->keypress(inchar);
            (*selected_win)->refresh();

            // also refresh monitor if refreshing signals view
            if(*selected_win == &win_signals)
                win_monitor.refresh();
        }
        else if(inchar == 'i' || inchar == 'j' || inchar == 'k' || inchar == 'l')
        {
            if(*selected_win == &win_monitor)
            {
                win_monitor.keypress(inchar);
                win_monitor.refresh();
            }
        }
        else if(inchar == '\n')
        {
            if( (*selected_win) == &win_explorer)
            {
                auto selected_sig = win_explorer.get_selected_signal();
                win_signals.add_signal(selected_sig);
                win_signals.refresh();
                win_monitor.refresh();
            }
        }
    }

    endwin();
    return 0;
}