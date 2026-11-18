# Localhost Web Edition — Verification Report

## Environment used

- GCC C99 shared-library build on Linux.
- Python standard-library HTTP server.
- HTTP smoke tests against `127.0.0.1:8765`.
- Node.js syntax check for `web/app.js`.

## Checks completed

| Check | Result |
|---|---|
| C bridge compiles with `-std=c99 -Wall -Wextra -Wpedantic` | PASS (no warnings) |
| Browser JavaScript parses (`node --check`) | PASS |
| Python server starts and serves `/api/health` | PASS |
| Homepage static file is served from localhost | PASS |
| `/api/state` returns valid JSON from C backend | PASS |
| Add customer through HTTP API | PASS |
| Add show through HTTP API | PASS |
| Book seat through HTTP API | PASS |
| Seat status updates in state response | PASS |
| C integration test: initial catalogue and fixed three screens | PASS |
| C integration test: duplicate phone and invalid phone | PASS |
| C integration test: three-screen assignment and conflict suggestion | PASS |
| C integration test: ticket pricing, food combo and promo | PASS |
| C integration test: no double booking | PASS |
| C integration test: sold-out waiting list and auto-promotion | PASS |
| C integration test: all six save files are written | PASS |
| Existing backend core regression suite (`tests/run_tests.sh`) | PASS |
| Existing backend console regression tests | PASS |

## Scope and limitation

The C backend and local HTTP/API path were exercised in this environment. The existing simplified backend core and CLI regression suites were also run successfully. The target Windows PC and Embarcadero Dev-C++ installation were not available here, so the first Windows DLL build and browser launch must still be confirmed on the user's computer. A headless Chromium visual check was blocked by the execution environment's browser policy; no claim of full Windows visual QA is made.

## Re-run tests

From the project folder:

```bash
python tests/test_local_api.py
node --check web/app.js
```

The C integration test uses a temporary data folder and does not alter the live project data files.
