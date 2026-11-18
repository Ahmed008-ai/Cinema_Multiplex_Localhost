#include "project.h"

/*
   All data is kept here so beginners can find the main arrays easily.
   The other .c files contain the functions that work with these arrays.
*/
Movie movies[MAX_MOVIES];
Screen screens[MAX_SCREENS];
Show shows[MAX_SHOWS];
Seat seats[MAX_SCREENS][MAX_SHOWS][ROWS][COLS];
Customer customers[MAX_CUSTOMERS];
FoodCombo combos[MAX_COMBOS];
char promoCodes[MAX_PROMOS][20];
float promoPercent[MAX_PROMOS];

Booking *bookings = NULL;
WaitEntry *waitList = NULL;

int movieCount = 0;
int showCount = 0;
int customerCount = 0;
int bookingCount = 0;
int bookingCapacity = 0;
int waitCount = 0;
int waitCapacity = 0;
int nextBookingNumber = 1;
int inputEnded = 0;

/* Keep the original project modular: main.c brings in each small module.
   In Dev-C++, compile main.c only. The other files must stay in this folder. */
#include "input.c"
#include "memory.c"
#include "movies.c"
#include "shows.c"
#include "seats.c"
#include "pricing.c"
#include "customers.c"
#include "storage.c"
#include "menus.c"

#ifndef CINEMA_TEST
int main(void)
{
    int choice;
    int i;

    /* Give each screen its fixed name and physical seat dimensions. */
    for (i = 0; i < MAX_SCREENS; i++)
    {
        screens[i].id = i;
        screens[i].rows = ROWS;
        screens[i].cols = COLS;
        sprintf(screens[i].name, "Screen %d", i + 1);
    }

    initAllSeats();
    initCombos();

    if (!initDynamicLists())
    {
        printf("Could not allocate memory. The program will close.\n");
        return 1;
    }

    /* Load saved information. If this is the first run, load sample movies. */
    if (!loadAll())
        loadSampleMovies();

    do
    {
        printMainMenu();
        choice = getIntInRange("Select an option: ", 1, 5);

        switch (choice)
        {
            case 1:
                adminMenu();
                break;
            case 2:
                boxOfficeMenu();
                break;
            case 3:
                customerMenu();
                break;
            case 4:
                saveAll();
                break;
            case 5:
                saveAll();
                printf("\nThank you for using Cinema Multiplex. Goodbye!\n");
                break;
        }
    } while (choice != 5 && !inputEnded);

    /* Also save if the user closes the input stream unexpectedly. */
    if (inputEnded)
        saveAll();

    free(bookings);
    free(waitList);
    return 0;
}
#endif /* CINEMA_TEST */
