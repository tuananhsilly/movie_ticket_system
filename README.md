# Movie Ticket System (C / TCP Client-Server)

This project is a socket-based movie ticket booking system implemented in C, designed with a custom text protocol, role-based access control, and modular server handlers.

## Project Summary

The system includes:
- **TCP server** handling multiple client requests through a request/response protocol
- **CLI client** for user interaction
- **Role-based operations** for Customer, Manager, and Admin
- **Core ticketing flow**: register, login, search movie, list shows, view seats, book seats, and view bookings
- **Admin/Manager operations** such as managing movies/shows and monitoring booking-related data

## Technical Highlights

- **Language & tooling:** C (GCC), Makefile build pipeline
- **Networking:** Custom TCP protocol with structured command/response format
- **Architecture:** Clear separation between client, server, handlers, and shared/common modules
- **Data handling:** Structured models and DB access layer in `src/common`
- **Security basics:** Authentication, session state checks, and role-based authorization before sensitive commands

## Repository Structure

- `src/client` – command-line client
- `src/server` – server core and request handlers
- `src/common` – shared models, protocol parser, DB helpers, utilities
- `docs/protocol-spec.md` – protocol contract and error/status conventions
- `docs/usecases.md` – business/use-case coverage
- `docs/state-machines` – state diagrams for login, booking, and admin flows

## Build & Run

```bash
cd /home/runner/work/movie_ticket_system/movie_ticket_system
make clean
make
./build/server
# In another terminal:
./build/client
```

## Example Capabilities Demonstrated

- User lifecycle: registration and login
- Movie discovery and show listing
- Seat-map retrieval with seat status tracking (FREE/BOOKED/HELD)
- Seat booking with validation
- Admin-level listing and monitoring of shows

## Matching Information for Multi-Model AI Things Lab (Employer Fit)

### Lab Values and Mission Alignment

1. **Interdisciplinary, proactive, and self-motivated**
   - This project required combining systems programming, networking, protocol design, and product workflow thinking.
   - I independently designed and integrated client, server, and shared modules to deliver an end-to-end working system.

2. **World-class research and education in applied AI/ML**
   - While this implementation is a software-engineering system, it is built as a strong foundation for applied AI/ML extensions.
   - The modular design can support recommendation, demand forecasting, anomaly detection, or intelligent scheduling experiments.

3. **Meaningful challenges, integrity, innovation, and rigorous computational methods**
   - The project addresses a real operational challenge: reliable ticket booking with role-aware access and seat-state consistency.
   - I applied structured protocol rules, explicit error handling, and clear state-driven logic to keep behavior correct and transparent.

4. **Bridge science and engineering for academic and societal impact**
   - The architecture translates abstract system design concepts into deployable software components.
   - It demonstrates how formal workflow/use-case modeling can become practical tools that improve real user-facing services.

5. **Leadership and leadership growth**
   - The codebase is organized for maintainability and collaboration through clear module boundaries and documentation.
   - This reflects my approach to leadership: building systems that others can understand, extend, and improve together.
