# CCA baseline and project requirement mapping

The uploaded CCA Ideas PDF states the baseline for every project as input
validation, switch-based menus with submenus, at least 10 functions, file save
and load, a recursive function, and an array of structures with sorting and
searching (page 1).

| Requirement | Where it is implemented |
|---|---|
| Switch-based main menu | `main.c` |
| Admin, Box Office, Customer Portal submenus | `menus.c` |
| Array of Movie, Screen, Show, Seat, Customer and Booking structures | `project.h`, `main.c` |
| Four-dimensional `seats[screen][show][row][col]` matrix | `project.h`, `seats.c` |
| Show-wise seat status and double-booking prevention | `seats.c` (`isSeatFree`, `createBooking`) |
| Fixed 3 physical screens and flexible date/time scheduling | `shows.c` |
| Movie-duration and 20-minute cleaning-gap conflict check | `shows.c` (`isScreenAvailable`) |
| Dynamic ticket pricing: seat category, day type, time and demand | `pricing.c` (`getTicketPrice`) |
| Food combos and calculated receipt | `pricing.c`, `seats.c` |
| FIFO waiting list and automatic promotion | `customers.c` |
| Refund tiers based on hours before showtime | `customers.c` (`hoursUntilShow`, `calculateRefund`) |
| Recursive loyalty-point total | `customers.c` (`calcLoyaltyPoints`) |
| Sort movies by revenue and shows by occupancy | `pricing.c` |
| Case-insensitive booking, phone and movie-title search | `pricing.c`, `customers.c`, `input.c` |
| Save/load bookings, seats, customers and loyalty points | `storage.c` |
| Dynamic booking and waiting-list arrays using `malloc` / `realloc` | `memory.c` |
| Input validation and error messages | `input.c` and all interactive modules |
| Duplicate phone, invalid seat, sold-out show and invalid promo checks | `customers.c`, `seats.c`, `pricing.c` |
| Pointers / pass-by-reference | `seats.c`, `customers.c`, `shows.c` |
| At least 10 modular functions | All source modules; see function prototypes in `project.h` |

## Notes for the presentation

- The seat map is a true 4-D array: `Seat seats[3][500][6][8]`.
- The physical screen count is fixed at 3. The show record limit is 500 in this
  beginner-friendly version because the CCA seat matrix is a fixed-size array.
- Showtimes are not limited to three named slots. The admin enters any valid
  `HH:MM` time and date.
- A show can only be placed on a screen when its time interval does not overlap
  another show on that screen, including the cleaning gap.
- The project uses a recursive function for loyalty points, as requested.

## CCA concept coverage checklist

| CCA concept | Cinema project example |
|---|---|
| Variables, data types and operators | Prices, discounts, occupancy percentages, refund calculations and time arithmetic |
| Input / output | Validated menus, formatted movie/show tables, seat maps and receipts |
| `if / else` and nested conditions | Screen conflicts, dynamic ticket pricing, refunds and sold-out checks |
| `switch` | Main menu and Admin / Box Office / Customer Portal submenus |
| Loops | Repeating menus, seat traversal, searches, sorting and input retries |
| 1-D arrays | Movie, screen, show and customer arrays; sorting index arrays |
| 2-D arrays | Rows and columns of a selected show's seat map |
| Strings | Movie titles, customer names, phone numbers, booking IDs and promo codes |
| Functions | Separate functions for scheduling, booking, pricing, searching, saving and reports |
| Pointers / pass by reference | Booking pointers, `realloc`, and customer-point updates through `int *` |
| Recursion | `calcLoyaltyPoints()` recursively totals active booking points |
| Structures | Movie, Screen, Show, Seat, Customer, Booking and FoodCombo |
| Sorting and searching | Bubble sort by revenue / occupancy; case-insensitive searches |
| File handling | Six text files store and restore the cinema data |
| Dynamic memory | `malloc` and `realloc` grow booking and waiting-list arrays |
| Error handling | Invalid input, missing/corrupt files, duplicate phone, occupied seat, invalid promo and sold-out show |
