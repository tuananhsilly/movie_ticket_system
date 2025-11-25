# Use Cases

## User Use Cases

### UC1: User Registration
- Actor: Guest User
- Precondition: User is not logged in
- Main Flow:
  1. User provides username, password, and role
  2. System validates input
  3. System creates new user account
  4. System confirms registration

### UC2: User Login
- Actor: Registered User
- Precondition: User has an account
- Main Flow:
  1. User provides username and password
  2. System validates credentials
  3. System creates session
  4. System grants access

### UC3: Search Movies
- Actor: Authenticated User
- Precondition: User is logged in
- Main Flow:
  1. User enters search keyword
  2. System searches movie database
  3. System returns matching movies
  4. User views results

### UC4: Book Ticket
- Actor: Authenticated User
- Precondition: User is logged in, show exists
- Main Flow:
  1. User selects a show
  2. User views available seats
  3. User selects seats
  4. System validates seat availability
  5. System creates booking
  6. System confirms booking

### UC5: View My Bookings
- Actor: Authenticated User
- Precondition: User is logged in
- Main Flow:
  1. User requests booking list
  2. System retrieves user's bookings
  3. System displays bookings

## Admin Use Cases

### UC6: Add Movie
- Actor: Admin
- Precondition: Admin is logged in
- Main Flow:
  1. Admin provides movie details
  2. System validates input
  3. System adds movie to database
  4. System confirms addition

### UC7: Add Show
- Actor: Admin
- Precondition: Admin is logged in, movie exists
- Main Flow:
  1. Admin selects movie
  2. Admin provides show details (datetime, hall)
  3. System validates input
  4. System creates show
  5. System initializes seat layout
  6. System confirms creation

### UC8: View Statistics
- Actor: Admin
- Precondition: Admin is logged in
- Main Flow:
  1. Admin requests statistics
  2. System calculates stats
  3. System displays statistics





