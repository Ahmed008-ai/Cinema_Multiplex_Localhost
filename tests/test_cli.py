"""Black-box checks for the beginner-friendly console interface.
Run from the project folder with: python tests/test_cli.py
"""
import datetime
import os
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXE = ROOT / ("cinema.exe" if os.name == "nt" else "cinema")

# Build the console program if the user has not built it yet.
if not EXE.exists():
    subprocess.run(
        ["gcc", "-std=c99", "-Wall", "-Wextra", "-o", str(EXE), str(ROOT / "backend" / "main.c")],
        check=True,
        cwd=ROOT,
    )


def run_app(data: str, folder: pathlib.Path) -> str:
    result = subprocess.run(
        [str(EXE)], input=data, text=True, capture_output=True,
        cwd=folder, timeout=10
    )
    if result.returncode != 0:
        raise AssertionError(f"Program returned {result.returncode}:\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="cinema_cli_") as temp:
        folder = pathlib.Path(temp)

        # First run: missing files, schedule a show, register, buy food and use a promo.
        show_date = (datetime.date.today() + datetime.timedelta(days=30)).isoformat()
        script = "\n".join([
            "1", "2", "0", "2026-02-31", show_date, "25:00", "10:00", "1", "0", "7",
            "3", "1", "Alice Example", "abc", "03001234567", "5",
            "2", "1", "0", "Z", "A", "9", "1", "03001234567",
            "1", "1", "BADCODE", "SAVE10", "7", "5", ""
        ])
        output = run_app(script, folder)
        assert "No movie data file found" in output
        assert "Use a real date" in output
        assert "24-hour HH:MM format" in output
        assert "Phone number must contain 7 to 15 digits only" in output
        assert "Show scheduled successfully" in output
        assert "Please enter one row letter from A to F" in output
        assert "Please enter a value from 1 to 8" in output
        assert "Invalid promo code" in output
        assert "TOTAL PAID" in output
        assert "BK0001" in output
        assert "All cinema data saved successfully" in output

        # Second run: saved data is loaded and the booked seat remains booked.
        reload_output = run_app("2\n3\n0\n7\n5\n", folder)
        assert "Saved cinema data loaded successfully" in reload_output
        assert "[X]" in reload_output

        # Search uses case-insensitive booking ID, phone and movie title matching.
        search_script = "\n".join([
            "2", "4", "1", "bk0001", "4", "2", "03001234567",
            "4", "3", "INCEPTION", "7", "5", ""
        ])
        search_output = run_app(search_script, folder)
        assert "BK0001" in search_output
        assert "Alice Example" in search_output
        assert "Inception" in search_output

    # A separate clean folder checks duplicate phone validation in one session.
    with tempfile.TemporaryDirectory(prefix="cinema_duplicate_") as temp:
        folder = pathlib.Path(temp)
        script = "\n".join([
            "3", "1", "Alice", "03001234567",
            "1", "Bob", "03001234567", "5", "5", ""
        ])
        output = run_app(script, folder)
        assert "Registration successful" in output
        assert "already registered" in output

    # Invalid menu text and out-of-range values must be rejected and retried.
    with tempfile.TemporaryDirectory(prefix="cinema_validation_") as temp:
        output = run_app("abc\n9\n5\n", pathlib.Path(temp))
        assert "Please enter a whole number" in output
        assert "Please enter a value from 1 to 5" in output

    # A corrupt optional schedule file must be reported, not crash the program.
    with tempfile.TemporaryDirectory(prefix="cinema_corrupt_") as temp:
        folder = pathlib.Path(temp)
        (folder / "movies.txt").write_text(
            "3\n0|Inception|148|0\n1|Interstellar|169|0\n2|Joyland|126|0\n"
        )
        (folder / "shows.txt").write_text("9999\n")
        output = run_app("5\n", folder)
        assert "shows.txt is damaged" in output
        assert "Goodbye" in output

    print("ALL CLI TESTS PASSED.")


if __name__ == "__main__":
    main()
