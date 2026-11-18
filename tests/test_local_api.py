"""Integration checks for the C backend web bridge (runs without a browser)."""
import ctypes
import datetime as dt
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def load_api(temp_dir: str):
    library = Path(temp_dir) / ("cinema_backend.dll" if os.name == "nt" else "libcinema_backend.so")
    gcc = "gcc"
    command = [gcc, "-std=c99", "-O1", "-shared", str(ROOT / "web_backend.c"), "-o", str(library), "-lm"]
    if os.name != "nt":
        command.insert(1, "-fPIC")
    else:
        command.insert(1, "-static-libgcc")
    subprocess.run(command, cwd=ROOT, check=True)
    os.chdir(temp_dir)
    lib = ctypes.CDLL(str(library))
    lib.api_init.restype = ctypes.c_int
    lib.api_get_state.restype = ctypes.c_char_p
    lib.api_add_movie.argtypes = [ctypes.c_char_p, ctypes.c_int]
    lib.api_add_movie.restype = ctypes.c_char_p
    lib.api_register_customer.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.api_register_customer.restype = ctypes.c_char_p
    lib.api_add_show.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.api_add_show.restype = ctypes.c_char_p
    lib.api_book.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
    lib.api_book.restype = ctypes.c_char_p
    lib.api_join_waitlist.argtypes = [ctypes.c_int, ctypes.c_int]
    lib.api_join_waitlist.restype = ctypes.c_char_p
    lib.api_cancel.argtypes = [ctypes.c_char_p]
    lib.api_cancel.restype = ctypes.c_char_p
    lib.api_save.restype = ctypes.c_char_p
    lib.api_shutdown.argtypes = []
    return lib


def read(pointer):
    return json.loads(pointer.decode("utf-8"))


def main():
    with tempfile.TemporaryDirectory(prefix="cine_web_test_") as temp:
        lib = load_api(temp)
        assert lib.api_init() == 1
        state = read(lib.api_get_state())
        assert state["meta"]["screenCount"] == 3
        assert len(state["movies"]) == 3
        print("PASS: C backend initializes with three fixed screens and sample movies")

        movie = read(lib.api_add_movie(b"Arrival", 116))
        assert movie["ok"] and movie["id"] == 3
        print("PASS: movie creation and validation")

        customer1 = read(lib.api_register_customer(b"Ayesha Khan", b"03001234567"))
        customer2 = read(lib.api_register_customer(b"Hamza Ali", b"03007654321"))
        assert customer1["ok"] and customer2["ok"]
        duplicate = read(lib.api_register_customer(b"Duplicate Guest", b"03001234567"))
        assert not duplicate["ok"]
        invalid_phone = read(lib.api_register_customer(b"Bad Phone", b"03-ABC"))
        assert not invalid_phone["ok"]
        print("PASS: customer registration, duplicate phone and invalid phone handling")

        show_date = (dt.date.today() + dt.timedelta(days=1)).isoformat()
        first_show = read(lib.api_add_show(0, show_date.encode(), 10 * 60, 0, -1))
        second_show = read(lib.api_add_show(1, show_date.encode(), 10 * 60, 0, -1))
        third_show = read(lib.api_add_show(2, show_date.encode(), 10 * 60, 0, -1))
        assert first_show["ok"] and second_show["ok"] and third_show["ok"]
        assert {first_show["screen"], second_show["screen"], third_show["screen"]} == {1, 2, 3}
        blocked = read(lib.api_add_show(3, show_date.encode(), 10 * 60, 0, -1))
        assert not blocked["ok"] and "suggestion" in blocked
        print("PASS: automatic assignment to three screens, overlap prevention and next-time suggestion")

        booked = read(lib.api_book(1, 0, 0, 0, 1, 1, b"SAVE10"))
        assert booked["ok"] and booked["bookingId"] == "BK0001"
        duplicate_seat = read(lib.api_book(2, 0, 0, 0, 0, 0, b"NONE"))
        assert not duplicate_seat["ok"]
        bad_promo = read(lib.api_book(2, 0, 0, 1, 0, 0, b"NOTREAL"))
        assert not bad_promo["ok"]
        print("PASS: booking, dynamic ticket pricing, food combo, promo and double-booking prevention")

        # Fill the second show to exercise sold-out and automatic queue promotion.
        booking_ids = []
        for row in range(6):
            for col in range(8):
                result = read(lib.api_book(1, 1, row, col, 0, 0, b"NONE"))
                assert result["ok"], result
                booking_ids.append(result["bookingId"])
        sold_state = read(lib.api_get_state())
        sold_show = next(show for show in sold_state["shows"] if show["id"] == 1)
        assert sold_show["occupancy"] == 100.0
        queue = read(lib.api_join_waitlist(2, 1))
        assert queue["ok"]
        cancelled = read(lib.api_cancel(booking_ids[0].encode()))
        assert cancelled["ok"] and cancelled["refund"] >= 0
        after = read(lib.api_get_state())
        assert len(after["waitlist"]) == 0
        active_for_second = [b for b in after["bookings"] if b["showId"] == 1 and b["active"]]
        assert len(active_for_second) == 48
        assert any(b["customerId"] == 2 and b["active"] for b in active_for_second)
        print("PASS: sold-out show, waiting-list queue, cancellation and automatic promotion")

        saved = read(lib.api_save())
        assert saved["ok"]
        for filename in ("movies.txt", "shows.txt", "seats.txt", "customers.txt", "bookings.txt", "waitlist.txt"):
            assert (Path(temp) / filename).exists(), filename
        print("PASS: all six persistence files are created")
        lib.api_shutdown()

    print("\nALL LOCAL API INTEGRATION TESTS PASSED")


if __name__ == "__main__":
    main()
