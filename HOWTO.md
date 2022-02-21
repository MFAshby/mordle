how to do the leaderboard?

i guess to start with, how to do scores?

easy enough:
- average over time period.
- treat missing days as a loss (score of 7? score of 10? how to weight this?)

so, when do we need it? you'll probably want to see, 

who is in the lead right now, 

and once you've played, where's your position in the list,

so... how do you store it reasonably efficiently, and update it on demand.

Idea 1, naievely.
- calculate it from the raw guesses table.
- calclate the score per day
- if there are no guesses, the score is 10
- if any guess matches the wordle, it's a win, otherwise it's a lose
- if it's a lose, the score is 7
- otherwise the score is the number of guesses
- take the average score over the last N days, including today (probably do a weekly, monthly, etc)

- pros: nothing else to store, normalized data still
- cons: complex query
- cons: split logic, either win or loss is now encoded both in the code (for page render) and in the database (for score calc)

Idea 2, store game outcome
- end of a game, store the result
- use that to calculate as per the above.

- pros: overcomes split logic, win/lose is contained only in the code.
- cons: some denormalization of data, risks getting out of sync
- cons: still a reasonably complicated query to get the stats?