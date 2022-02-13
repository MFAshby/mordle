mordle
======

Martin's Wordle Clone. [Original game](https://powerlanguage.co.uk/wordle)

My game requires login and stores scores on the server, so I can have a leaderboard. Also store the wordlist serverside to prevent cheating.

# Design

## Frontend

classic ui:
- grid of tiles 5x6, 
- on screen keyboard,
- highlight green for right-letter right-place
- highlight yellow for right-letter wrong-place

state on the client: current guess.
minimal frontend code; listen to keyboard and fill in the latest row.
form submit: post the guess, server will handle the rest.

## Backend

use a database, use the following tables:

wordlist 
- word

answer
- date
- ref_word

user
- name
- passwordhash
- passwordhashsalt

session
- ref_user
- session_token
- csrftoken

game
- ref_user
- ref_answer

guess
- ref_game
- word
- valid?
- timestamp

everything else can be derived from this 

Can be implemented this way with minimal JS, so I guess:
- Boring login stuff
- GET /
server-side generated page since the whole game state is on the server.
- POST /guess

# Modules

Just a single source file for now I suppose.

I'll need to 
1. ensure the database is initialized
2. start an http server

http server will... 
serve generated content on /
handle signup/login forms submit on /signup /login
handle form submit for guesses on /guess

generated page will show:
signup / login forms if not logged in
current game when logged in
  win/lose page if already won or lost
  current guesses, with appropriate highlight if in progress
  stats (current streak, average scores etc etc)


Core components then I guess: 
- view model for the page, represents all the data rendered
array of guesses in the current game, 
with matching / part / unmatching characters
that's it, I suppose

- SQL query to fetch the current game, guess, etc?

# Tech choices

plain old javascript for interactivity
c
mongoose http server
mustach html templating
postgres database

