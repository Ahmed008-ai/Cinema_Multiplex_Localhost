#ifndef PROJECT_H
#define PROJECT_H

/*
   CINEMA MULTIPLEX MANAGEMENT SYSTEM
   Beginner-friendly C project for Dev-C++.

   The program uses simple structures, arrays, loops, functions,
   pointers, recursion and text files as required by the CCA baseline.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* Physical screens stay fixed. Shows can be scheduled at different times. */
#define MAX_MOVIES 50
#define MAX_SCREENS 3
#define MAX_SHOWS 500
#define MAX_CUSTOMERS 200
#define ROWS 6
#define COLS 8
#define MAX_COMBOS 4
#define MAX_PROMOS 3
#define CLEANING_MINUTES 20

/* ------------------------- STRUCTURES ------------------------- */

typedef struct
{
    int id;
    char title[50];
    int duration;              /* Movie length in minutes */
    float revenue;              /* Net revenue collected for this movie */
} Movie;

typedef struct
{
    int id;
    char name[20];
    int rows;
    int cols;
} Screen;

typedef struct
{
    int id;
    int screenId;               /* 0, 1 or 2 */
    int movieId;
    char date[11];              /* YYYY-MM-DD */
    int startMinutes;           /* Minutes after midnight */
    int dayType;                /* 0 = weekday, 1 = weekend */
    int duration;
    int bookedSeats;
    int totalSeats;
} Show;

typedef struct
{
    int status;                 /* 0 = free, 1 = booked */
    int category;               /* 1 = Silver, 2 = Gold, 3 = Platinum */
} Seat;

typedef struct
{
    int id;
    char name[40];
    char phone[16];
    int points;
} Customer;

typedef struct
{
    char id[12];
    int customerId;
    int showId;
    int row;
    int col;
    float ticketPrice;
    float foodCost;
    float discount;
    float totalPaid;
    int active;
} Booking;

typedef struct
{
    int customerId;
    int showId;
} WaitEntry;

typedef struct
{
    int id;
    char name[30];
    float price;
} FoodCombo;

/* ------------------------- GLOBAL DATA ------------------------- */

extern Movie movies[MAX_MOVIES];
extern Screen screens[MAX_SCREENS];
extern Show shows[MAX_SHOWS];
extern Seat seats[MAX_SCREENS][MAX_SHOWS][ROWS][COLS];
extern Customer customers[MAX_CUSTOMERS];
extern FoodCombo combos[MAX_COMBOS];
extern char promoCodes[MAX_PROMOS][20];
extern float promoPercent[MAX_PROMOS];

/* Booking and waiting-list arrays grow with realloc when they become full. */
extern Booking *bookings;
extern WaitEntry *waitList;

extern int movieCount;
extern int showCount;
extern int customerCount;
extern int bookingCount;
extern int bookingCapacity;
extern int waitCount;
extern int waitCapacity;
extern int nextBookingNumber;
extern int inputEnded;

/* ------------------------- INPUT FUNCTIONS ------------------------- */
int getIntInRange(const char prompt[], int min, int max);
float getFloatInRange(const char prompt[], float min, float max);
void getText(const char prompt[], char text[], int size);
void getPhone(const char prompt[], char phone[]);
void getDate(const char prompt[], char date[]);
int getTimeMinutes(const char prompt[]);
int parseDate(const char date[], struct tm *result);
void minutesToText(int minutes, char text[]);
int equalsIgnoreCase(const char a[], const char b[]);
void toLowerText(char text[]);

/* ------------------------- MEMORY FUNCTIONS ------------------------- */
int initDynamicLists(void);
int growBookings(void);
int growWaitList(void);

/* ------------------------- MOVIE FUNCTIONS ------------------------- */
void addMovie(void);
void viewMovies(void);
void loadSampleMovies(void);

/* ------------------------- SHOW FUNCTIONS ------------------------- */
int createShow(int movieId, const char date[], int startMinutes,
               int dayType, int preferredScreen);
int isScreenAvailable(int screenId, const char date[], int startMinutes,
                      int duration);
int findNextAvailableTime(int movieId, const char date[], int startMinutes,
                          int *screenId, int *suggestedMinutes);
void addShow(void);
void viewShows(void);
int selectShow(void);
void setDayType(void);

/* ------------------------- SEAT AND BOOKING FUNCTIONS ------------------------- */
void initAllSeats(void);
void initSeatsForShow(int showId);
void displaySeatMap(int showId);
int isValidSeat(int row, int col);
int isSeatFree(int showId, int row, int col);
int countBookedSeats(int showId);
float getOccupancy(int showId);
int isShowSoldOut(int showId);
void freeSeat(int showId, int row, int col);
int addBooking(int customerId, int showId, int row, int col,
               float ticketPrice, float foodCost, float discount);
int createBooking(int customerId, int showId, int row, int col,
                  float foodCost, float promoPercentValue);
int findBookingById(const char bookingId[]);
void bookSeat(void);
void viewBookingsOfShow(int showId);

/* ------------------------- PRICING AND REPORT FUNCTIONS ------------------------- */
float getBasePrice(int category);
int getSurgeLevel(float occupancy);
float getTicketPrice(int showId, int category);
void showPriceBoard(int showId);
void initCombos(void);
float selectFoodCombo(void);
float getPromoPercent(const char code[]);
void printReceipt(int bookingIndex);
void updateRevenue(int movieId, float amount);
void sortMoviesByRevenue(void);
void sortShowsByOccupancy(void);
void searchBooking(void);
void searchCustomer(void);
void searchMovie(void);

/* ------------------------- CUSTOMER, REFUND AND WAITING LIST ------------------------- */
void registerCustomer(void);
int isDuplicatePhone(const char phone[]);
int findCustomerByPhone(const char phone[]);
void cancelBooking(void);
float cancelBookingRecord(int bookingIndex, int hoursRemaining);
float calculateRefund(float amount, int hoursRemaining);
int hoursUntilShow(int showId);
int addToWaitList(int customerId, int showId);
void promoteFromWaitList(int showId, int row, int col);
void viewWaitList(int showId);
int calcLoyaltyPoints(int customerId, int index); /* Recursive function */
int getLoyaltyTier(int points);
void updateCustomerPoints(int *points, int newPoints);
void viewCustomerProfile(void);

/* ------------------------- MENU FUNCTIONS ------------------------- */
void adminMenu(void);
void boxOfficeMenu(void);
void customerMenu(void);
void printHeader(const char title[]);
void printMainMenu(void);

/* ------------------------- FILE FUNCTIONS ------------------------- */
void saveAll(void);
int loadAll(void);
void rebuildSeatsFromBookings(void);

#endif
