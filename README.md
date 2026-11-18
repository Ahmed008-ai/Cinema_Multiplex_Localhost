# CINÉ — Cinema Multiplex Management Studio (Localhost Web Edition)

A polished browser-based frontend for the existing beginner-friendly C cinema backend.
The website runs at **http://127.0.0.1:8765** on your own computer. It is not a public website and does not require hosting or an internet connection for its core features.

## What is included

- `web/index.html` — page structure and application sections.
- `web/styles.css` — responsive dark-sidebar / light-workspace design system.
- `web/app.js` — browser interactions and calls to the local API.
- `server.py` — small local web server using only Python's standard library.
- `web_backend.c` — thin C bridge that exposes the existing backend functions to the browser server.
- `backend/` — the same simplified C backend modules and console entry point from the previous simple version. The files `project.h`, `input.c`, `memory.c`, `movies.c`, `shows.c`, `seats.c`, `pricing.c`, `customers.c`, `storage.c`, `menus.c`, and `main.c` are retained as-is.
- `tests/test_local_api.py` — integration tests for the browser-to-C backend API.
- `docs/` — the CCA baseline mapping and previous backend test report.

## Requirements on Windows

1. Windows 10 or Windows 11.
2. Python 3.8 or newer.
3. GCC. The launcher looks for GCC on PATH and in common Embarcadero Dev-C++ 6.x TDM-GCC folders.
4. A modern browser (Chrome, Edge, Firefox, etc.).

No Node.js, npm, Flask, Django, pip packages, or VS Code are required.

### If Python is not installed

Install Python 3 from <https://www.python.org/downloads/>. During setup, tick **Add Python to PATH**. Then run the launcher again.

### If GCC is not found

Open Dev-C++ and check **Tools → Compiler Options** to confirm the TDM-GCC 9.2 compiler is installed. The launcher searches the common Embarcadero Dev-C++ installation folders automatically. If your installation uses a different folder, add its `bin` folder (the one containing `gcc.exe`) to the Windows PATH and reopen the launcher.

## Run the website — beginner steps

1. Extract the ZIP to a simple folder, for example `C:\CinemaMultiplexWeb`.
2. Open the extracted `Cinema_Multiplex_Localhost` folder.
3. Double-click **`start_localhost.bat`**.
4. The first run compiles `web_backend.c` and the existing backend modules into a local DLL. This is done automatically.
5. A terminal window will display the local address. Your browser should open automatically at **http://127.0.0.1:8765**.
6. Keep the terminal window open while using the website. Closing it stops the local server.
7. To stop the application safely, click the terminal window and press **Ctrl+C**. The backend saves the records before closing.

If the browser does not open automatically, copy this address into Chrome or Edge:

`http://127.0.0.1:8765`

## How the frontend and backend connect

- The browser loads the HTML, CSS and JavaScript from the local Python server.
- JavaScript sends JSON requests to `http://127.0.0.1:8765/api/...`.
- Python passes those requests to the C DLL using `ctypes`.
- The C bridge calls the existing backend functions such as `createShow`, `createBooking`, `getTicketPrice`, `cancelBookingRecord`, and `addToWaitList`.
- The C backend remains responsible for seat status, scheduling conflicts, dynamic prices, discounts, loyalty points, cancellation refunds and waiting-list promotion.

The Python file is a local HTTP transport only; it does not replace the C booking logic.

## Data and backups

The backend saves these text files in the same folder as `start_localhost.bat`:

- `movies.txt`
- `shows.txt`
- `seats.txt`
- `customers.txt`
- `bookings.txt`
- `waitlist.txt`

Back up these files before replacing the project folder or changing backend data formats. Do not delete them if you want to keep bookings and customer records.

## Using the application

- **Overview:** current catalogue, shows, active bookings, revenue and screen occupancy.
- **Movie library:** add and browse movies.
- **Showtimes:** schedule screenings on exactly three physical screens. The backend checks runtime overlap and the 20-minute cleaning gap; it suggests another time if all screens are busy.
- **Ticket desk:** select a show, choose an available seat, select a customer and optional food combo/promo, then confirm the booking.
- **Customers:** register guests and view loyalty points/tiers.
- **Insights:** review movie revenue and show occupancy.
- **Booking history:** cancel active bookings; the C backend applies the tiered refund and promotes the first waiting customer when appropriate.

Promo codes: `SAVE10`, `CINEMA20`, `STUDENT15`.

## Open/edit source in Dev-C++

Dev-C++ is useful for editing the C backend. Open `web_backend.c` or files inside `backend/` with **File → Open**. The HTML/CSS/JavaScript files are in `web/`; they can be opened in a text editor.

For the localhost edition, do **not** press Compile & Run on `gui_main.c` from the previous Win32 experiment. This package uses a browser frontend, not a native Windows GUI. Run `start_localhost.bat` instead. When you edit a C backend file, stop the server and start it again; the launcher recompiles the DLL when a source file has changed.

## Tests

Run the integration checks from a terminal in this folder:

```bash
python tests/test_local_api.py
```

The test creates its own temporary data folder and does not overwrite your live cinema records. It checks initialization, three-screen scheduling, conflict handling, customer validation, booking, double-booking prevention, promo validation, sold-out handling, waiting-list promotion and persistence files.

The test report records the checks run in this environment. A final run on the target Windows PC is still recommended before the CCA presentation.
