# Test report

## Build checks

| Check | Result |
|---|---|
| GCC C99 build with `-Wall -Wextra -Wpedantic -Werror` | Passed |
| Clang C99 build with `-Wall -Wextra -Wpedantic -Werror` | Passed |
| GCC `-fanalyzer` static analysis | Passed |
| Clang static analysis | Passed |
| AddressSanitizer + UndefinedBehaviorSanitizer core run | Passed |
| Leak detection in sanitizer run | Passed |

## Automated core tests

`tests/test_core.c` checks:

1. Missing-file handling on a first run.
2. Date validation, including an impossible calendar date.
3. Three simultaneous shows are assigned to the three existing screens.
4. A conflicting fourth show is rejected; no fourth screen is created.
5. A later show can reuse a screen after the movie and cleaning interval.
6. A seat is unique to its show and cannot be double-booked.
7. Weekday morning, weekend evening and demand-surge pricing.
8. Food cost, promo percentage and final paid amount calculations.
9. Invalid promo code handling.
10. Booking-array growth beyond its initial capacity.
11. Case-insensitive booking ID and phone lookup.
12. Loyalty points calculated recursively.
13. Waiting-list growth beyond its initial capacity.
14. Cancellation refund tiers and automatic waiting-list promotion.
15. A show becomes sold out after all 48 seats are booked.
16. Saving and reloading movies, shows, bookings, seats, customers, loyalty
    points and waiting-list entries.
17. Movie revenue sorting and show occupancy sorting.

The automated scenario grows the booking list to 98 records and the waiting
queue to 8 entries (beyond its initial capacity), then reloads the saved data.

## Automated console tests

`tests/test_cli.py` checks:

- First-run startup when data files are missing.
- Scheduling a show and assigning a screen.
- Invalid calendar date, invalid time and invalid phone input being rejected.
- Invalid seat row and column input being rejected.
- Invalid promo code being rejected before a valid code is accepted.
- Customer registration and duplicate-phone rejection.
- Receipt generation and persistence after restarting the program.
- Case-insensitive search by booking ID, customer phone and movie title.
- Invalid menu text and out-of-range menu choices.
- A corrupt optional show file being reported without crashing.

## Scope and limitations

The tests passed in the provided Linux build environment. GCC and Clang builds
were tested; Dev-C++ on the user's Windows computer was not directly available
for testing. The project includes a Dev-C++ setup guide and a Windows build
batch file, but the user should still compile and test it in their own Dev-C++
installation before the class presentation.

No finite test suite can prove the absence of every possible defect. The report
records the exact checks performed rather than claiming that bugs are
impossible.
