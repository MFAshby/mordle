## Bugs

Broken form submit, crashes the game [mitigated with always-restart policy]

CSRF protection

Store hash of sessions?

## Features

Stats

Leaderboard
- push the win/lose/score calculation down to the database. 
- Maybe use functions to do it?
- Even a custom C function if you want to share code...
- OR, calculate it on guess. If a game is won/lost, record that.
   yes it's denormalization
   live with it?

Game history?

Kindle/old browser support

i18n, a11y

Name change, delete account.

--- non-functional ---

Sqlite backend?

Configure database location?

Configure listen port?

Docker image?

database init / migrate?

## Tests

selenium tests?

valgrind tests

some kind of CI or at least a script to run locally to test everything

## Code cleanup

