#include <ncurses.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

// Function prototypes
typedef struct {
    void (*stop)(void*);
    void* context;
} StopHandle;

typedef StopHandle (*MenuCallback)(WINDOW*, const char*);

StopHandle menu_callback(WINDOW* content, const char* selected);
StopHandle animated_callback(WINDOW* content, const char* selected);

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
                return options[choice].display_text;
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
    WINDOW* content = NULL;
    StopHandle current_stop = {0};
    
    // Initialize content window
    content = newwin(max_y, max_x-menu_width-1, 0, menu_width+1);
    if (!content) {
        move(orig_y, orig_x);
        refresh();
        return NULL;
    }
    keypad(content, TRUE);
    
    while(1) {
        // Draw menu items
        for(int i = 0; i < num_items; i++) {
            if(i == choice) attron(A_REVERSE);
            mvprintw(i, 1, "%s", options[i].display_text);
            if(i == choice) attroff(A_REVERSE);
        }
        refresh();
        
        // Call the callback for the current choice
        if (current_stop.stop) {
            fprintf(stderr, "DEBUG: Calling current stop function\n");
            current_stop.stop(current_stop.context);
            current_stop.stop = NULL;
            if (current_stop.context) {
                free(current_stop.context);
                current_stop.context = NULL;
            }
            fprintf(stderr, "DEBUG: Stop function completed\n");
            if (content) {
                wclear(content);
                wrefresh(content);
            }
        }
        
        if (content) {
            current_stop = options[choice].callback(content, options[choice].display_text);
            wrefresh(content);
        }
        
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
                fprintf(stderr, "DEBUG: Handling menu option change\n");
                if (current_stop.stop) {
                    fprintf(stderr, "DEBUG: Calling current_stop()\n");
                    current_stop.stop(current_stop.context);
                    current_stop.stop = NULL;
                    if (current_stop.context) {
                        free(current_stop.context);
                        current_stop.context = NULL;
                    }
                    fprintf(stderr, "DEBUG: current_stop() completed\n");
                }
                if (content) {
                    fprintf(stderr, "DEBUG: Deleting content window\n");
                    werase(content);
                    wrefresh(content);
                    delwin(content);
                    content = NULL;
                    fprintf(stderr, "DEBUG: Content window deleted\n");
                }
                move(orig_y, orig_x);
                refresh();
                return options[choice].display_text;
        }
    }
}

StopHandle menu_callback(WINDOW* content, const char* selected) {
    if(strcmp(selected, "Quit") == 0) {
        return (StopHandle){0};
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
    
    // Return empty stop handle
    return (StopHandle){0};
}

// Animation context structure
typedef struct {
    pthread_t thread;
    WINDOW* win;
    int* keep_running;
    pthread_mutex_t lock;
} AnimationContext;

void* animation_thread(void* arg) {
    fprintf(stderr, "DEBUG: Starting animation thread\n");
    if (!arg) {
        fprintf(stderr, "ERROR: NULL arg in animation_thread\n");
        return NULL;
    }
    
    AnimationContext* ctx = (AnimationContext*)arg;
    
    pthread_mutex_lock(&ctx->lock);
    if (!ctx->win || !ctx->keep_running) {
        fprintf(stderr, "ERROR: Invalid animation context\n");
        pthread_mutex_unlock(&ctx->lock);
        return NULL;
    }
    
    WINDOW* win = ctx->win;
    volatile int* keep_running = ctx->keep_running;
    pthread_mutex_unlock(&ctx->lock);
    fprintf(stderr, "DEBUG: Animation thread initialized\n");

    int x = 0;
    int dir = 1;
    int width = getmaxx(win) - 10;
    
    while(1) {
        pthread_mutex_lock(&ctx->lock);
        int should_run = (keep_running && *keep_running);
        pthread_mutex_unlock(&ctx->lock);
        
        if (!should_run) {
            fprintf(stderr, "DEBUG: Animation thread exiting\n");
            break;
        }
        
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

// Global stop function
void stop_animation(void* context) {
    fprintf(stderr, "DEBUG: Entering stop_animation()\n");
    if (!context) {
        fprintf(stderr, "DEBUG: NULL context in stop_animation\n");
        return;
    }
    
    AnimationContext* ctx = (AnimationContext*)context;
    
    // Lock before accessing shared resources
    pthread_mutex_lock(&ctx->lock);
    
    // Signal thread to stop
    if (ctx->keep_running) {
        fprintf(stderr, "DEBUG: Setting keep_running to 0\n");
        *(ctx->keep_running) = 0;
    }
    
    // Unlock after modifying shared resources
    pthread_mutex_unlock(&ctx->lock);
    
    // Wait for thread to finish
    if (ctx->thread) {
        fprintf(stderr, "DEBUG: Joining thread\n");
        pthread_join(ctx->thread, NULL);
        fprintf(stderr, "DEBUG: Thread joined\n");
    }
    
    // Clean up resources
    pthread_mutex_lock(&ctx->lock);
    if (ctx->keep_running) {
        fprintf(stderr, "DEBUG: Freeing keep_running\n");
        free(ctx->keep_running);
        ctx->keep_running = NULL;
    }
    pthread_mutex_unlock(&ctx->lock);
    
    pthread_mutex_destroy(&ctx->lock);
    //free(ctx); //doublefree
}

StopHandle animated_callback(WINDOW* content, const char* selected) {
    fprintf(stderr, "DEBUG: Entering animated_callback()\n");
    if (!content) {
        fprintf(stderr, "ERROR: NULL content window in animated_callback\n");
        return (StopHandle){0};
    }
    
    // Allocate and initialize context
    AnimationContext* ctx = calloc(1, sizeof(AnimationContext));
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to allocate animation context\n");
        return (StopHandle){0};
    }

    pthread_mutex_init(&ctx->lock, NULL);
    ctx->keep_running = calloc(1, sizeof(int));
    if (!ctx->keep_running) {
        fprintf(stderr, "ERROR: Failed to allocate keep_running flag\n");
        pthread_mutex_destroy(&ctx->lock);
        //free(ctx); double free
        return (StopHandle){0};
    }
    *(ctx->keep_running) = 1;

    // Create animation thread
    pthread_t thread;

    // Pass context directly to thread
    ctx->win = content;
    if (pthread_create(&thread, NULL, animation_thread, ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to create animation thread\n");
        free(ctx->keep_running);
        free(ctx);
        return (StopHandle){0};
    }
    
    // Setup return handle
    ctx->thread = thread;
    return (StopHandle){ .stop = stop_animation, .context = ctx };
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
                const char* choice = show_menu2(menu_items, num_items);
                printw("Selected: %s\n", choice);
            } else {
                const char* choice = show_menu1(menu_items, num_items);
                printw("Selected: %s\n", choice);
            }
        }
    }
    
    // Cleanup
    reset_shell_mode();
    delscreen(screen);
    return 0;
}
