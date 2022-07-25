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
    ExplorerWin(int height, int width, int y, int x): Window(height, width, y, x, "Explorer")
    {}

    void populate()
    {
        std::vector<VCDScope*> scopes = *global_trace->get_scopes();

        unsigned line_no=0;
        for(unsigned scope_indx=0; scope_indx < scopes.size(); scope_indx++)
        {
            // print scope name
            move(line_no++, 1);
            wattron(win, A_BOLD);
            wprintw(win, " %s", scopes[scope_indx]->name.c_str());
            wattroff(win, A_BOLD);

            auto signals = scopes[scope_indx]->signals;

            for(int sig_indx=0; sig_indx < signals.size(); sig_indx++)
            {
                move(line_no++, 1);
                wprintw(win, "  %s [%d:0]", signals[sig_indx]->reference.c_str(), signals[sig_indx]->size);
            }
        }
    }

    void keypress(char key)
    {}
};

class SignalsWin: public Window
{
public:
    SignalsWin(int height, int width, int y, int x): Window(height, width, y, x, "Signals")
    {}

    void populate() {}

    void keypress(char key)
    {}
};

class MonitorWin: public Window
{
public:
    MonitorWin(int height, int width, int y, int x): Window(height, width, y, x, "Monitor")
    {}

    void populate() {}

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
    
    
    std::vector<Window *> avail_windows = {&win_explorer, &win_signals, &win_monitor};
    int selected_win_indx = 0;

    while (1)
    {
        Window * selected_win = avail_windows[selected_win_indx];

        // print headers of all windows (bold if selected)
        for(auto itr: avail_windows)
            itr->_printheader(selected_win==itr);

        // populate wins
        for(auto itr: avail_windows)
            itr->populate();


        // read input
        char inchar = getch();
        if(inchar == 'q')
        {
            break;
        }
        else if(inchar == 'd')  // select right window
        {
            if(selected_win_indx < avail_windows.size()-1)
                selected_win_indx++;
        }
        else if(inchar == 'a')  // select left window
        {
            if(selected_win_indx > 0)
                selected_win_indx--;
        }
        else 
        {
            selected_win->keypress(inchar);
        }
        
       
        wrefresh(stdscr);
        win_explorer.refresh();
        win_signals.refresh();
        win_monitor.refresh();
    }

    endwin();
    return 0;
}