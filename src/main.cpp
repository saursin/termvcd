#include <ncurses.h>
#include <vector>
#include "window.hpp"

#include "../lib/vcd_parser/VCDFileParser.hpp"
#include "../lib/vcd_parser/VCDFile.hpp"
#include "../lib/vcd_parser/VCDTypes.hpp"

// Global var
VCDFile * global_trace = nullptr;

class ExplorerWin: public Window
{
public:
    unsigned selected_line_indx = 0, max_line_cnt = 0;
    std::vector<VCDScope*> scopes;
    std::vector<bool> scope_is_expanded;

    ExplorerWin(int height, int width, int y, int x): Window(height, width, y, x, "Explorer")
    {
        scopes = *global_trace->get_scopes();
        
        // all scopes are not expanded initially
        for (auto s:scopes)
            scope_is_expanded.push_back(false);
    }

    void print()
    {
        unsigned voffset = 1;
        unsigned line = 0;
        for(unsigned scope_indx=0; scope_indx < scopes.size(); scope_indx++)
        {
            // print scope name
            move(line+voffset, 1);
            
            if (selected_line_indx == line) 
                wattron(win, COLOR_PAIR(1));
            
            wattron(win, A_BOLD);
            wprintw(win, " %s", (scope_indx==0 ? "*" :scopes[scope_indx]->name.c_str()));
            wattroff(win, A_BOLD);

            if (selected_line_indx == line)
                wattroff(win, COLOR_PAIR(1));

            line++;
            auto signals = scopes[scope_indx]->signals;

            if(scope_is_expanded[scope_indx])
            {
                for(int sig_indx=0; sig_indx < signals.size(); sig_indx++)
                {
                    move(line+voffset, 1);
                    if (selected_line_indx == line)
                        wattron(win, COLOR_PAIR(1));
                    
                    wprintw(win, "  %s [%d:0]", signals[sig_indx]->reference.c_str(), signals[sig_indx]->size);
                    
                    if (selected_line_indx == line)
                        wattroff(win, COLOR_PAIR(1));
                    
                    line++;
                }
            }
        }

        // save last line no
        max_line_cnt = line;
    }

    void keypress(char key)
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
            scope_is_expanded[selected_line_indx] = !scope_is_expanded[selected_line_indx];
        }

        // redraw
        print();
    }
};

class SignalsWin: public Window
{
public:
    SignalsWin(int height, int width, int y, int x): Window(height, width, y, x, "Signals")
    {}

    void print() {}

    void keypress(char key)
    {}
};

class MonitorWin: public Window
{
public:
    MonitorWin(int height, int width, int y, int x): Window(height, width, y, x, "Monitor")
    {}

    void print() {}

    void keypress(char key)
    {}
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

    if(trace == nullptr)
        return -1;

    // initialize program
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

    // get stdscr dimensions
    int stdscr_height, stdscr_width;
    getmaxyx(stdscr, stdscr_height, stdscr_width);

    // Print header
    std::string header = "*****   Terminal VCD viewer   *****";
    wmove(stdscr, 0, (stdscr_width/2) -(header.length()/2));    // center align
    wprintw(stdscr, "%s", header.c_str());
    
    // Print filename
    wmove(stdscr, 1, 1);
    wprintw(stdscr, "File: %s", filepath.c_str());

    // Windows
    ExplorerWin win_explorer(stdscr_height-4, stdscr_width / 8, 3, 1);
    SignalsWin win_signals(stdscr_height-4, stdscr_width / 8, 3, win_explorer.width+1);
    MonitorWin win_monitor(stdscr_height-4, stdscr_width *(6 / 8), 3, win_explorer.width + win_signals.width+1);
    
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
        else if(inchar == 'w' || inchar == 's' || inchar == 'e')
        {
            (*selected_win)->keypress(inchar);
            (*selected_win)->refresh();
        }
    }

    endwin();
    return 0;
}