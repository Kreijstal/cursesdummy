#include <ncurses.h>
#include <string.h>
#include <pthread.h>

// Function prototypes
typedef void (*StopFunction)();
typedef StopFunction (*MenuCallback)(WINDOW*, const char*);

StopFunction menu_callback(WINDOW* content, const char* selected);
StopFunction animated_callback(WINDOW* content, const char* selected);

typedef struct {
    const char* display_text;
    MenuCallback callback;
} MenuOption;

const char* show_menu1(MenuOption* options, int num_items) {
    // Create selection menu
    WINDOW* menu = newwin(10, 30, 5, 5);
    box(menu, 0, 0);
    keypad(menu, TRUE);
    
    int choice = 0;
    
    while(1) {
        for(int i = 0; i < num_items; i++) {
            if(i == choice) wattron(menu, A_REVERSE);
            mvwprintw(menu, i+1, 2, "%s", options[i].display_text);
            if(i == choice) wattroff(menu, A_REVERSE);
        }
        wrefresh(menu);
        
        int ch = wgetch(menu);
        switch(ch) {
            case KEY_UP: 
                choice = (choice > 0) ? choice-1 : num_items-1;
                break;
            case KEY_DOWN:
                choice = (choice < num_items-1) ? choice+1 : 0;
                break;
            case 10: // Enter key
                werase(menu);
                wrefresh(menu);
                delwin(menu);
                callback(options[choice]);
                return;
        }
    }
}

const char* show_menu2(MenuOption* options, int num_items) {
    // Save current cursor position
    int orig_y, orig_x;
    getyx(stdscr, orig_y, orig_x);
    
    // Clear screen and setup panes
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int menu_width = max_x / 3;
    
    // Create border between panes
    for(int y = 0; y < max_y; y++) {
        mvaddch(y, menu_width, ACS_VLINE);
    }
    
    int choice = 0;
    // Draw vertical divider and menu border
    for(int y = 0; y < max_y; y++) {
        mvaddch(y, menu_width, ACS_VLINE);
    }
    refresh(); // Make sure divider is visible
    WINDOW* content = newwin(max_y, max_x-menu_width-1, 0, menu_width+1);
    StopFunction current_stop = NULL;
    
    while(1) {
        // Draw menu items
        for(int i = 0; i < num_items; i++) {
            if(i == choice) attron(A_REVERSE);
            mvprintw(i, 1, "%s", options[i].display_text);
            if(i == choice) attroff(A_REVERSE);
        }
        
        // Call the callback for the current choice
        if (current_stop) {
            current_stop();
            current_stop = NULL;
        }
        
        wclear(content);
        current_stop = options[choice].callback(content, options[choice].display_text);
        wrefresh(content);
        
        refresh();
        
        int ch = getch();
        switch(ch) {
            case KEY_UP:
                choice = (choice > 0) ? choice-1 : num_items-1;
                break;
            case KEY_DOWN:
                choice = (choice < num_items-1) ? choice+1 : 0;
                break;
            case 10: // Enter key
                if (current_stop) {
                    current_stop();
                }
                delwin(content);
                move(orig_y, orig_x);
                refresh();
                return options[choice].display_text;
        }
    }
}

StopFunction menu_callback(WINDOW* content, const char* selected) {
    if(strcmp(selected, "Quit") == 0) {
        return NULL;
    }
    
    // Clear and show prominent selection
    wclear(content);
    wattron(content, A_BOLD);
    mvwprintw(content, 0, 0, ">> %s <<", selected);
    wattroff(content, A_BOLD);
    
    // Add separator line
    int max_x = getmaxx(content);
    for(int x = 0; x < max_x; x++) {
        mvwaddch(content, 1, x, ACS_HLINE);
    }
    
    // Example content - can be replaced with actual implementation
    mvwprintw(content, 3, 0, "Details will appear here");
    wrefresh(content);
    
    // Return a simple stop function
    return (StopFunction)NULL;
}

// Animation thread function
void* animation_thread(void* arg) {
    WINDOW* win = (WINDOW*)arg;
    volatile int* keep_running = (volatile int*)arg;
    int x = 0;
    int dir = 1;
    int width = getmaxx(win) - 10;
    
    while(*keep_running) {
        wclear(win);
        mvwprintw(win, 0, 0, ">> Animation <<");
        mvwprintw(win, 2, x, "====>");
        wrefresh(win);
        
        x += dir;
        if(x >= width || x <= 0) dir *= -1;
        
        napms(100); // 100ms delay
    }
    return NULL;
}

StopFunction animated_callback(WINDOW* content, const char* selected) {
    volatile int keep_running = 1;
    
    // Create animation thread with both window and keep_running flag
    pthread_t thread;
    struct {
        WINDOW* win;
        volatile int* running;
    } thread_args = {content, &keep_running};
    pthread_create(&thread, NULL, animation_thread, &thread_args);
    
    // Return stop function
    void stop_animation() {
        keep_running = 0;
        pthread_join(thread, NULL);
    }
    
    return stop_animation;
}

int main() {
    // Initialize ncurses
    SCREEN* screen = newterm(NULL, stdout, stdin);
    set_term(screen);
    keypad(stdscr, TRUE);
    echo();
    scrollok(stdscr, TRUE);
    
    // Setup menu system
    MenuOption menu_items[] = {
        {"Option 1", menu_callback},
        {"Option 2", menu_callback},
        {"Option 3", menu_callback},
        {"Animation", animated_callback},
        {"Quit", menu_callback}
    };
    int num_items = sizeof(menu_items)/sizeof(MenuOption);
    
    // Main REPL loop
    char input[256];
    while(1) {
        printw("> ");
        getstr(input);
        printw("You entered: %s\n", input);
        
        if(strcmp(input, "exit") == 0) {
            break;
        }
        else if(strcmp(input, "select") == 0) {
            printw("Choose menu style (1 or 2): ");
            getstr(input);
            if(strcmp(input, "2") == 0) {
                const char* choice = show_menu2(menu_items, num_items, menu_callback);
                printw("Selected: %s\n", choice);
            } else {
                const char* choice = show_menu1(menu_items, num_items, menu_callback);
                printw("Selected: %s\n", choice);
            }
        }
    }
    
    // Cleanup
    reset_shell_mode();
    delscreen(screen);
    return 0;
}
