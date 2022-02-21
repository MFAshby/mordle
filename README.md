mordle
======

Martin's Wordle Clone. Hosted at [wordle.mfashby.net](https://wordle.mfashby.net).

[Original game](https://powerlanguage.co.uk/wordle)

My game requires login and stores scores on the server, so I can have a leaderboard. Also store the wordlist serverside to prevent cheating.

# Design principles

- push all the state to the database
- separate concerns to different modules

# Modules / Interfaces

- models.h: common structures used throughout the application, e.g. game_state, user, leaderboard.
- storage.h: storage layer, hide it's implementation so we could swap it (e.g. in-memory, sqlite, postgres etc etc.)
- user.h: logic for signup, login, etc.
- game.h: logic for game manouvres
- index_page.h: function to render index page, similar for any other pages (but you might only require the one)
- main.c: HTTP server and routing

# Tech choices
keepin it old-school
- c
- mongoose http server
- mustach html templating
- postgres database
- maybe a sprinkle of javascript
- munit for testing
